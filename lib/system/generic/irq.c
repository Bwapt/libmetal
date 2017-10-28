/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Xilinx nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * @file	generic/irq.c
 * @brief	generic libmetal irq definitions.
 */

#include <errno.h>
#include "metal/irq.h"
#include "metal/sys.h"
#include "metal/log.h"
#include "metal/mutex.h"
#include "metal/list.h"
#include "metal/utilities.h"
#include "metal/alloc.h"

/** IRQ handlers descriptor structure */
struct metal_irq_hddesc {
	metal_irq_handler hd;     /**< irq handler */
	void *drv_id;             /**< id to identify the driver
	                               of the irq handler */
	struct metal_device *dev; /**< device identifier */
	struct metal_list list;   /**< handler list container */
};

/** IRQ descriptor structure */
struct metal_irq_desc {
	int irq;                      /**< interrupt number */
	struct metal_irq_hddesc hdls; /**< interrupt handlers */
	struct metal_list list;       /**< interrupt list container */
};

/** IRQ state structure */
struct metal_irqs_state {
	struct metal_irq_desc hds; /**< interrupt descriptors */
	unsigned int intr_enable;  /**< interrupt enable */
	metal_mutex_t irq_lock;    /**< access lock */
};

static struct metal_irqs_state _irqs;

int metal_irq_register(int irq,
                       metal_irq_handler hd,
                       struct metal_device *dev,
                       void *drv_id)
{
	struct metal_irq_desc *hdd_p = NULL;
	struct metal_irq_hddesc *hdl_p;
	struct metal_list *node;
	unsigned int irq_flags_save;

	if (irq < 0) {
		metal_log(METAL_LOG_ERROR, "%s: irq %d need to be a positive number\n",
		          __func__, irq);
		return -EINVAL;
	}

	if ((drv_id == NULL) || (hd == NULL)) {
		metal_log(METAL_LOG_ERROR, "%s: irq %d need drv_id and hd.\n",
		          __func__, irq);
		return -EINVAL;
	}

	/* Search for irq in list */
	metal_mutex_acquire(&_irqs.irq_lock);
	metal_list_for_each(&_irqs.hds.list, node) {
		hdd_p = metal_container_of(node, struct metal_irq_desc, list);

		if (hdd_p->irq == irq) {
			struct metal_list *h_node;

			/* Check if drv_id already exist */
			metal_list_for_each(&hdd_p->hdls.list, h_node) {
				hdl_p = metal_container_of(h_node, struct metal_irq_hddesc, list);

				/* if drv_id already exist reject */
				if ((hdl_p->drv_id == drv_id) &&
					((dev == NULL) || (hdl_p->dev == dev))) {
					metal_log(METAL_LOG_ERROR, "%s: irq %d already registered."
							"Will not register again.\n",
							__func__, irq);
					metal_mutex_release(&_irqs.irq_lock);
					return -EINVAL;
				}
			}
			/* irq found and drv_id not used, get out of metal_list_for_each */
			break;
		}
	}

	/* Either need to add handler to an existing list or to a new one */
	hdl_p = metal_allocate_memory(sizeof(struct metal_irq_hddesc));
	if (hdl_p == NULL) {
		metal_log(METAL_LOG_ERROR,
		          "%s: irq %d cannot allocate mem for drv_id %d.\n",
		          __func__, irq, drv_id);
		metal_mutex_release(&_irqs.irq_lock);
		return -ENOMEM;
	}
	hdl_p->hd = hd;
	hdl_p->drv_id = drv_id;
	hdl_p->dev = dev;

	/* interrupt already registered, add handler to existing list*/
	if ((hdd_p != NULL) && (hdd_p->irq == irq)) {
		irq_flags_save=metal_irq_save_disable();
		metal_list_add_tail(&hdd_p->hdls.list, &hdl_p->list);
		metal_irq_restore_enable(irq_flags_save);

		metal_log(METAL_LOG_DEBUG, "%s: success, irq %d add drv_id %p \n",
		          __func__, irq, drv_id);
		metal_mutex_release(&_irqs.irq_lock);
		return 0;
	}

	/* interrupt was not already registered, add */
	hdd_p = metal_allocate_memory(sizeof(struct metal_irq_desc));
	if (hdd_p == NULL) {
		metal_log(METAL_LOG_ERROR, "%s: irq %d cannot allocate mem.\n",
		          __func__, irq);
		metal_mutex_release(&_irqs.irq_lock);
		return -ENOMEM;
	}
	hdd_p->irq = irq;
	metal_list_init(&hdd_p->hdls.list);
	metal_list_add_tail(&hdd_p->hdls.list, &hdl_p->list);

	irq_flags_save=metal_irq_save_disable();
	metal_list_add_tail(&_irqs.hds.list, &hdd_p->list);
	metal_irq_restore_enable(irq_flags_save);

	metal_log(METAL_LOG_DEBUG, "%s: success, added irq %d\n", __func__, irq);
	metal_mutex_release(&_irqs.irq_lock);
	return 0;
}

/* helper function for metal_irq_unregister() */
static void metal_irq_delete_node(struct metal_list **node, void *p_to_free)
{
	unsigned int irq_flags_save;

	*node = (*node)->prev;
	irq_flags_save=metal_irq_save_disable();
	metal_list_del((*node)->next);
	metal_irq_restore_enable(irq_flags_save);
	metal_free_memory(p_to_free);
}

int metal_irq_unregister(int irq,
                         metal_irq_handler hd,
                         struct metal_device *dev,
                         void *drv_id)
{
	struct metal_irq_desc *hdd_p;
	struct metal_list *node;

	if (irq < 0) {
		metal_log(METAL_LOG_ERROR, "%s: irq %d need to be a positive number\n",
		          __func__, irq);
		return -EINVAL;
	}

	/* Search for irq in list */
	metal_mutex_acquire(&_irqs.irq_lock);
	metal_list_for_each(&_irqs.hds.list, node) {

		hdd_p = metal_container_of(node, struct metal_irq_desc, list);

		if (hdd_p->irq == irq) {
			struct metal_list *h_node;
			struct metal_irq_hddesc *hdl_p;
			unsigned int delete_count = 0;

			metal_log(METAL_LOG_DEBUG, "%s: found irq %d\n", __func__, irq);

			/* Search through handlers */
			metal_list_for_each(&hdd_p->hdls.list, h_node) {
				hdl_p = metal_container_of(h_node, struct metal_irq_hddesc, list);

				if (((hd == NULL) || (hdl_p->hd == hd)) &&
				    ((drv_id == NULL) || (hdl_p->drv_id == drv_id)) &&
				    ((dev == NULL) || (hdl_p->dev == dev))) {
					metal_log(METAL_LOG_DEBUG,
					          "%s: unregister hd=%p drv_id=%p dev=%p\n",
						  __func__, hdl_p->hd, hdl_p->drv_id, hdl_p->dev);
					metal_irq_delete_node(&h_node, hdl_p);
					delete_count++;
				}
			}

			/* we did not find any handler to delete */
			if (!delete_count) {
				metal_log(METAL_LOG_DEBUG, "%s: No matching entry\n",
				          __func__);
				metal_mutex_release(&_irqs.irq_lock);
				return -ENOENT;

			}

			/* if interrupt handlers list is empty, unregister interrupt */
			if (metal_list_is_empty(&hdd_p->hdls.list)) {
				metal_log(METAL_LOG_DEBUG,
				          "%s: handlers list empty, unregister interrupt\n",
					  __func__);
				metal_irq_delete_node(&node, hdd_p);
			}

			metal_log(METAL_LOG_DEBUG, "%s: success\n", __func__);

			metal_mutex_release(&_irqs.irq_lock);
			return 0;
		}
	}

	metal_log(METAL_LOG_DEBUG, "%s: No matching IRQ entry\n", __func__);

	metal_mutex_release(&_irqs.irq_lock);
	return -ENOENT;
}

unsigned int metal_irq_save_disable(void)
{
	if (_irqs.intr_enable == 1) {
		sys_irq_save_disable();
		_irqs.intr_enable = 0;
	}

	return 0;
}

void metal_irq_restore_enable(unsigned int flags)
{
	(void)flags;

	if (_irqs.intr_enable == 0) {
		sys_irq_restore_enable();
		_irqs.intr_enable = 1;
	}
}

void metal_irq_enable(unsigned int vector)
{
        sys_irq_enable(vector);
}

void metal_irq_disable(unsigned int vector)
{
        sys_irq_disable(vector);
}

/**
 * @brief default handler
 */
void metal_irq_isr(unsigned int vector)
{
	struct metal_list *node;
	struct metal_irq_desc *hdd_p;

	metal_list_for_each(&_irqs.hds.list, node) {
		hdd_p = metal_container_of(node, struct metal_irq_desc, list);

		if ((unsigned int)hdd_p->irq == vector) {
			struct metal_list *h_node;
			struct metal_irq_hddesc *hdl_p;

			metal_list_for_each(&hdd_p->hdls.list, h_node) {
				hdl_p = metal_container_of(h_node, struct metal_irq_hddesc, list);

				(hdl_p->hd)(vector, hdl_p->drv_id);
			}
		}
	}
}

int metal_irq_init(void)
{
	/* list of interrupt having at least one handler registered */
	metal_list_init(&_irqs.hds.list);
	/* list of registered handlers for an interrupt */
	metal_list_init(&_irqs.hds.hdls.list);
	/* mutex to manage concurrent access to shared irq data */
	metal_mutex_init(&_irqs.irq_lock);
	return 0;
}

void metal_irq_deinit(void) 
{
	metal_mutex_deinit(&_irqs.irq_lock);
}

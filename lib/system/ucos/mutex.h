#ifndef __METAL_MUTEX__H__
#error "Include metal/mutex.h instead of metal/ucos/mutex.h"
#endif

#ifndef __METAL_UCOS_MUTEX__H__
#define __METAL_UCOS_MUTEX__H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // TODO : metal_mutex_t
} metal_mutex_t;

/*
 * METAL_MUTEX_DEFINE - used for defining and initializing a global or
 * static singleton mutex
 */
#define METAL_MUTEX_DEFINE(m) metal_mutex_t m = METAL_MUTEX_INIT(m)

static inline void __metal_mutex_init(metal_mutex_t *mutex) {
  // TODO : __metal_mutex_init
}

static inline void __metal_mutex_deinit(metal_mutex_t *mutex) {
  // TODO : __metal_mutex_deinit
}

static inline int __metal_mutex_try_acquire(metal_mutex_t *mutex) {
  // TODO : __metal_mutex_try_acquire
}

static inline void __metal_mutex_acquire(metal_mutex_t *mutex) {
  // TODO : __metal_mutex_acquire
}

static inline void __metal_mutex_release(metal_mutex_t *mutex) {
  // TODO : __metal_mutex_release
}

static inline void __metal_mutex_is_acquired(metal_mutex_t *mutex) {
  // TODO : __metal_mutex_is_acquired
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_UCOS_MUTEX__H__ */
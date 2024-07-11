#ifndef __METAL_ALLOC_H__
#error "Include metal/alloc.h instead of metal/ucos/alloc.h"
#endif

#ifndef __METAL_UCOS_ALLOC__H__
#define __METAL_UCOS_ALLOC__H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *__metal_allocate_memory(unsigned int size) {
  // TODO : __metal_allocate_memory
  return;
}

static inline void __metal_free_memory(void *ptr) {
  // TODO : __metal_free_memory
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_UCOS_ALLOC__H__ */
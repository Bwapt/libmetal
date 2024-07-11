#ifndef __METAL_SLEEP__H__
#error "Include metal/sleep.h instead of metal/ucos/sleep.h"
#endif

#ifndef __METAL_UCOS_SLEEP__H__
#define __METAL_UCOS_SLEEP__H__

#ifdef __cplusplus
extern "C" {
#endif

static inline int __metal_sleep_usec(unsigned int usec)
{
  // TODO : __metal_sleep_usec
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_UCOS_SLEEP__H__ */
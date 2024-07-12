#ifndef __METAL_ARM32_CPU__H__
#define __METAL_ARM32_CPU__H__

#include <metal/compiler.h>

static inline void metal_cpu_yield(void)
{
    __asm__ __volatile__("yield");    // TODO: metal_cpu_yield
}

#endif /* __METAL_ARM32_CPU__H__ */
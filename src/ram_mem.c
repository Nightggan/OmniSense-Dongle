// RAM-resident memcpy / memset / memmove for RP2350.
//
// RP2350's bootrom no longer exports ROM_FUNC_MEMCPY / ROM_FUNC_MEMSET, so
// newlib resolves them to flash-resident libgcc code.  On a dual-core build
// the XIP bus arbitration between core0 and core1 can cause audio glitches or
// hangs when both cores fetch instructions from flash simultaneously (e.g.
// during a config_save() flash erase).  These overrides ensure the three most
// frequently called memory primitives always execute from SRAM.

#include <stddef.h>
#include "pico/platform.h"

void * __not_in_flash_func(memcpy)(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void * __not_in_flash_func(memset)(void *dst, int c, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    while (n--) *d++ = (unsigned char)c;
    return dst;
}

void * __not_in_flash_func(memmove)(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

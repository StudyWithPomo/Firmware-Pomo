#pragma once

#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *format_time(long epochSeconds, const char *format);

// Copy src into dst (size bytes, always terminated), cutting at a UTF-8 character boundary
// so a long name never ends in half a character. A null src copies "".
static inline void copy_utf8(char *dst, size_t size, const char *src)
{
    if (size == 0) return;
    size_t n = 0;
    if (src) {
        while (n < size - 1 && src[n]) n++;
        // src[n] is the first byte left out; back up while it continues a character
        while (n > 0 && src[n] && ((unsigned char)src[n] & 0xC0) == 0x80) n--;
        memcpy(dst, src, n);
    }
    dst[n] = '\0';
}

#ifdef __cplusplus
}
#endif
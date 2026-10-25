#pragma once

#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *format_time(long epochSeconds, const char *format);

// Copy src into dst (size bytes, always terminated), cutting at a UTF-8 character boundary
// so a name never ends in half a character, whether the cut is ours or the sender's.
// A null src copies "".
static inline void copy_utf8(char *dst, size_t size, const char *src)
{
    if (size == 0) return;
    size_t n = 0;
    if (src) {
        while (n < size - 1 && src[n]) n++;
        // Find where the last character starts; drop it if it has fewer bytes than its lead
        // byte promises
        size_t start = n;
        while (start > 0 && ((unsigned char)src[start - 1] & 0xC0) == 0x80) start--;
        if (start > 0) {
            const unsigned char lead = (unsigned char)src[start - 1];
            const size_t need = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
            if (n - (start - 1) < need) n = start - 1;
        }
        memcpy(dst, src, n);
    }
    dst[n] = '\0';
}

#ifdef __cplusplus
}
#endif
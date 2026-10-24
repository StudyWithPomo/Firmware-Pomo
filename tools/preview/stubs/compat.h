#pragma once
// Force-included in the preview build: POSIX bits MinGW lacks
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
#if defined(_WIN32)
static inline size_t strlcat(char *dst, const char *src, size_t size)
{
    size_t d = strlen(dst), s = strlen(src);
    if (d + 1 < size) {
        size_t n = size - d - 1;
        if (n > s) n = s;
        memcpy(dst + d, src, n);
        dst[d + n] = '\0';
    }
    return d + s;
}
static inline int setenv(const char *name, const char *value, int) { return _putenv_s(name, value); }
// The preview's clock is UTC so screenshots don't depend on this PC's time zone
static inline struct tm *localtime_r(const time_t *t, struct tm *out) { return gmtime_s(out, t) == 0 ? out : NULL; }
#endif
#ifdef __cplusplus
}
#endif

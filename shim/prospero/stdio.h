#pragma once

#if defined(__has_include_next)
#  if __has_include_next(<stdio.h>)
#    include_next <stdio.h>
#  else
#    error "no stdio.h found for Prospero shim"
#  endif
#else
#  include_next <stdio.h>
#endif

#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef vasprintf
static inline int vasprintf(char** strp, const char* fmt, va_list ap)
{
    if (!strp || !fmt)
        return -1;

    va_list ap_len;
    va_copy(ap_len, ap);
    const int len = vsnprintf(NULL, 0, fmt, ap_len);
    va_end(ap_len);
    if (len < 0)
    {
        *strp = NULL;
        return -1;
    }

    const auto buf = (char*)malloc((size_t)len + 1);
    if (!buf)
    {
        *strp = NULL;
        return -1;
    }

    va_list ap_out;
    va_copy(ap_out, ap);
    const int len2 = vsnprintf(buf, (size_t)len + 1, fmt, ap_out);
    va_end(ap_out);
    if (len2 < 0)
    {
        free(buf);
        *strp = NULL;
        return -1;
    }

    *strp = buf;
    return len2;
}
#endif

#ifndef asprintf
static inline int asprintf(char** strp, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int r = vasprintf(strp, fmt, ap);
    va_end(ap);
    return r;
}
#endif

#ifdef __cplusplus
}
#endif
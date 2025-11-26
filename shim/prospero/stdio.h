//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include_next <stdio.h>

#include <stdarg.h>
#include <stdlib.h>

static inline int vasprintf(char** strp, const char* fmt, va_list ap)
{
    if (!strp || !fmt)
    {
        return -1;
    }

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

static inline int asprintf(char** strp, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int r = vasprintf(strp, fmt, ap);
    va_end(ap);
    return r;
}

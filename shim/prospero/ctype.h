#pragma once

#include_next <ctype.h>

static inline int isascii(const int c)
{
    return (c & ~0x7f) == 0;
}
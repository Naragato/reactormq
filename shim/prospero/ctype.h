#pragma once

#if defined(__has_include_next)
#  if __has_include_next(<ctype.h>)
#    include_next <ctype.h>
#  else
#    error "no ctype.h found for Prospero shim"
#  endif
#else
#  include_next <ctype.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef isascii
static inline int isascii(const int c)
{
    return (c & ~0x7f) == 0;
}
#endif

#ifdef __cplusplus
}
#endif
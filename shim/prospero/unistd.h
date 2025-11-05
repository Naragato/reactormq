#pragma once

#if defined(__has_include_next)
#  if __has_include_next(<unistd.h>)
#    include_next <unistd.h>
#  else
#    error "no unistd.h found for Prospero shim"
#  endif
#else
#  include_next <unistd.h>
#endif

#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{

#endif

#ifndef _SC_PAGESIZE
#define _SC_PAGESIZE 1
#endif

static inline long sysconf(const int name)
{
    if (name == _SC_PAGESIZE)
        return 4096;
    return -1;
}

/* Only provide a stub if nobody has macro-wrapped getprogname.
   LibreSSL defines `#define getprogname libressl_getprogname`,
   so this block will be skipped there. */
#ifndef getprogname
static inline const char* getprogname(void)
{
    return "prospero";
}
#endif

/* getuid stub is still fine here – Prospero's unistd.h doesn't declare it. */
static inline uid_t getuid(void)
{
    return (uid_t)0;
}

#ifdef __cplusplus
}
#endif
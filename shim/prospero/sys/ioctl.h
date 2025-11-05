#pragma once

#if defined(__has_include_next)
#  if __has_include_next(<sys/ioctl.h>)
#    include_next <sys/ioctl.h>
#    define RMQ_HAVE_REAL_SYS_IOCTL 1
#  endif
#endif

#ifndef RMQ_HAVE_REAL_SYS_IOCTL
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{

#endif

#ifndef FIONBIO
#define FIONBIO 0x8004667e
#endif

static inline int ioctl(const int fd, const unsigned long request, ...)
{
    (void)fd;
    (void)request;
    return -1;
}

#ifdef __cplusplus
}
#endif

#endif
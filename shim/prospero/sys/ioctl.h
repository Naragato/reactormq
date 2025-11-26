#pragma once

#include <sys/types.h>

#ifndef FIONBIO
#define FIONBIO 0x8004667e
#endif

static inline int ioctl(const int fd, const unsigned long request, ...)
{
    (void)fd;
    (void)request;
    return -1;
}
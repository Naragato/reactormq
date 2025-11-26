//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include_next <unistd.h>

#include <sys/types.h>


#ifndef _SC_PAGESIZE
#define _SC_PAGESIZE 1
#endif

static inline long sysconf(const int name)
{
    if (name == _SC_PAGESIZE)
        return 4096;
    return -1;
}

#ifndef getprogname
static inline const char* getprogname(void)
{
    return "prospero";
}
#endif

static inline uid_t getuid(void)
{
    return 0;
}
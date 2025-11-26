//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include_next <sys/dirent.h>

typedef struct DIR DIR;

static DIR* opendir(const char* name)
{
    (void)name;
    return NULL;
}

static struct dirent* readdir(const DIR* dirp)
{
    (void)dirp;
    return NULL;
}

static int closedir(const DIR* dirp)
{
    (void)dirp;
    return 0;
}
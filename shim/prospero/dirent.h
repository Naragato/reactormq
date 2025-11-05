#pragma once

#if defined(__has_include_next)
#  if __has_include_next(<sys/dirent.h>)
#    include_next <sys/dirent.h>
#    define RMQ_HAVE_REAL_DIRENT 1
#  else
#    error "no dirent.h found for Prospero shim"
#  endif
#else
#  include_next <dirent.h>
#  define RMQ_HAVE_REAL_DIRENT 1
#endif

#ifdef RMQ_HAVE_REAL_DIRENT

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{

#endif

typedef struct DIR DIR;

static DIR* opendir(const char* name)
{
    (void)name;
    return NULL;
}

static struct dirent* readdir(DIR* dirp)
{
    (void)dirp;
    return NULL;
}

static int closedir(DIR* dirp)
{
    (void)dirp;
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif
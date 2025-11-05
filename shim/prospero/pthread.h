#pragma once

#if defined(__has_include_next)
#  if __has_include_next(<pthread.h>)
#    include_next <pthread.h>
#    define RMQ_HAVE_REAL_PTHREAD 1
#  endif
#endif

#ifndef RMQ_HAVE_REAL_PTHREAD
#  include_next <pthread.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*_rmq_atfork_fn)(void);

static inline int pthread_atfork(
    const _rmq_atfork_fn prepare,
    const _rmq_atfork_fn parent,
    const _rmq_atfork_fn child)
{
    (void)prepare;
    (void)parent;
    (void)child;
    return 0;
}

#ifdef __cplusplus
}
#endif
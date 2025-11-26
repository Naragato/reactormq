//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include_next <pthread.h>

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
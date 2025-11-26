//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7

#ifndef LOG_USER
#define LOG_USER (1 << 3)
#endif

#ifndef LOG_LOCAL0
#define LOG_LOCAL0 (16 << 3)
#define LOG_LOCAL1 (17 << 3)
#define LOG_LOCAL2 (18 << 3)
#define LOG_LOCAL3 (19 << 3)
#define LOG_LOCAL4 (20 << 3)
#define LOG_LOCAL5 (21 << 3)
#define LOG_LOCAL6 (22 << 3)
#define LOG_LOCAL7 (23 << 3)
#endif

#ifndef LOG_PID
#define LOG_PID 0x01
#endif
#ifndef LOG_CONS
#define LOG_CONS 0x02
#endif
#ifndef LOG_NDELAY
#define LOG_NDELAY 0x08
#endif
#ifndef LOG_NOWAIT
#define LOG_NOWAIT 0x10
#endif

static inline void openlog(const char* ident, const int option, const int facility)
{
    (void)ident;
    (void)option;
    (void)facility;
}

static inline void syslog(const int priority, const char* fmt, ...)
{
    (void)priority;
    (void)fmt;
}

static inline void closelog(void) {}
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <sys/socket.h>

struct hostent
{
    char* h_name;
    char** h_aliases;
    int h_addrtype;
    int h_length;
    char** h_addr_list;
};

#define h_addr h_addr_list[0]

struct addrinfo
{
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    struct sockaddr* ai_addr;
    char* ai_canonname;
    struct addrinfo* ai_next;
};

#ifndef AI_PASSIVE
#define AI_PASSIVE 0x0001
#endif

#ifndef EAI_FAIL
#define EAI_FAIL -1
#endif
#ifndef EAI_NONAME
#define EAI_NONAME -2
#endif
#ifndef EAI_AGAIN
#define EAI_AGAIN -3
#endif

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif
#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 0x0002
#endif
#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV 0x0008
#endif

static inline void freeaddrinfo(struct addrinfo* ai)
{
    (void)ai;
}

static inline int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res)
{
    (void)node;
    (void)service;
    (void)hints;
    (void)res;
    return EAI_FAIL;
}

static inline const char* gai_strerror(const int ecode)
{
    (void)ecode;
    return "getaddrinfo not supported";
}

static inline int getnameinfo(
    const struct sockaddr* sa,
    const socklen_t salen,
    const char* host,
    const size_t hostlen,
    const char* serv,
    const size_t servlen,
    const int flags)
{
    (void)sa;
    (void)salen;
    (void)host;
    (void)hostlen;
    (void)serv;
    (void)servlen;
    (void)flags;
    return EAI_FAIL;
}

static inline struct hostent* gethostbyname(const char* name)
{
    (void)name;
    return NULL;
}

static inline struct hostent* gethostbyaddr(const void* addr, const socklen_t len, const int type)
{
    (void)addr;
    (void)len;
    (void)type;
    return NULL;
}
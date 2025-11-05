#pragma once

#if defined(__has_include_next)
#  if __has_include_next(<sys/socket.h>)
#    include_next <sys/socket.h>
#    define RMQ_HAVE_REAL_SYS_SOCKET 1
#  endif
#endif

#ifndef RMQ_HAVE_REAL_SYS_SOCKET
#  error "no sys/socket.h found for Prospero shim"
#endif

#ifndef SOMAXCONN
#define SOMAXCONN 16
#endif

#ifndef RMQ_HAVE_RMQ_SOCKADDR_IN6
#define RMQ_HAVE_RMQ_SOCKADDR_IN6

#include <stdint.h>

struct in6_addr
{
    unsigned char s6_addr[16];
};

struct sockaddr_in6
{
    sa_family_t sin6_family;
    uint16_t sin6_port;
    uint32_t sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t sin6_scope_id;
};

#endif
#pragma once

#include_next <sys/socket.h>
#include <stdint.h>

#ifndef SOMAXCONN
#define SOMAXCONN 16
#endif

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
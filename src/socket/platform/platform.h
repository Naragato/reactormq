#pragma once

#if REACTORMQ_SOCKET_WITH_WINSOCKET

#include <winsock2.h>
#include <ws2tcpip.h>

using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocketHandle = INVALID_SOCKET;

#elif REACTORMQ_WITH_SOCKET_POLYFILL

#include "reactormq/shim/socket.h"

#else

#include <unistd.h>
#include <sys/socket.h>

#if REACTORMQ_SOCKET_WITH_IOCTL
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

#if REACTORMQ_SOCKET_WITH_POLL
#include <poll.h>
#endif

#include <arpa/inet.h>
#include <netinet/in.h>

#if REACTORMQ_SOCKET_WITH_GETHOSTNAME
#include <netdb.h>
#endif

#if REACTORMQ_SOCKET_WITH_NODELAY
#include <netinet/tcp.h>
#endif

#if REACTORMQ_PLATFORM_HAS_BSD_TIME
#include <sys/time.h>
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

using SOCKLEN = socklen_t;
using SocketHandle = int;
constexpr SocketHandle kInvalidSocketHandle = -1;
using SOCKADDR_IN = sockaddr_in;
using TIMEVAL = timeval;

#if REACTORMQ_SOCKET_WITH_IOCTL
#define ioctlsocket ioctl
#endif

inline int closesocket(const SocketHandle socket)
{
    ::shutdown(socket, SHUT_RDWR);
    return ::close(socket);
}

#endif

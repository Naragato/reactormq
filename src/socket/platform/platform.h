//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#if REACTORMQ_WITH_UE5

#include "SocketSubsystem.h"
#include "Sockets.h"

using SocketHandle = FSocket*;
inline constexpr SocketHandle kInvalidSocketHandle = nullptr;

#elif REACTORMQ_SOCKET_WITH_WINSOCKET

#include <WS2tcpip.h>
#include <WinSock2.h>

using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocketHandle = INVALID_SOCKET;

#elif REACTORMQ_WITH_SOCKET_POLYFILL

#include "reactormq/shim/socket.h"

#else

#include <sys/socket.h>
#include <unistd.h>

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
inline constexpr int SOCKET_ERROR = -1;
#endif

#ifndef INVALID_SOCKET
inline constexpr int INVALID_SOCKET = -1;
#endif

using SOCKLEN = socklen_t;
using SocketHandle = int;
inline constexpr SocketHandle kInvalidSocketHandle = -1;
using SOCKADDR_IN = sockaddr_in;
using TIMEVAL = timeval;

#if REACTORMQ_SOCKET_WITH_IOCTL
inline int ioctlsocket(const SocketHandle s, const unsigned long req, void* argp)
{
    return ioctl(s, req, argp);
}
#endif

inline int closesocket(const SocketHandle socket)
{
    shutdown(socket, SHUT_RDWR);
    return ::close(socket);
}

#endif
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::socket
{
    /**
     * @brief Cross-platform socket error codes.
     * Normalized error values that map from platform-specific codes
     * (for example, errno on POSIX or WSAGetLastError on Windows).
     * If SocketError::Unknown is returned, inspect the underlying platform
     * error via getLastErrorCode() (or the platform equivalent) for details.
     */
    enum class SocketError : int32_t
    {
        None = 0, ///< No error.
        WouldBlock, ///< Operation would block (e.g. non-blocking socket with no data).
        InProgress, ///< Operation is in progress (e.g. non-blocking connect).
        AlreadyInProgress, ///< Another operation is already in progress on this socket.
        Interrupted, ///< Operation interrupted by a signal or similar.
        ConnectionReset, ///< Connection was forcibly closed by the remote peer.
        ConnectionAborted, ///< Connection was aborted locally.
        TimedOut, ///< Connection attempt or operation timed out.
        ConnectionRefused, ///< Remote side refused the connection (no listener / actively refused).
        NotConnected, ///< Operation requiring a connection was attempted on a non-connected socket.
        AddressInUse, ///< Address is already in use (e.g. bind() on a port that’s taken).
        AddressNotAvailable, ///< The specified address is not valid for this host or interface.
        NetworkDown, ///< Local network interface is down.
        NetworkUnreachable, ///< Network is unreachable from this host.
        HostUnreachable, ///< Host is unreachable.
        HostNotFound, ///< DNS: host name not found.
        TryAgain, ///< DNS: temporary failure, try again.
        NoRecovery, ///< DNS: unrecoverable error.
        NoData, ///< DNS: no records of the requested type found.
        MessageTooLong, ///< Message is too long to be sent atomically.
        AccessDenied, ///< Operation not permitted (e.g. due to insufficient privileges).
        InvalidArgument, ///< One or more arguments were invalid.
        NotSocket, ///< Descriptor is not a socket.
        OperationNotSupported, ///< Operation not supported on this socket / protocol / address family.
        ProtocolError, ///< Generic protocol error.
        ProtocolNotSupported, ///< Protocol not supported.
        AddressFamilyNotSupported, ///< Address family not supported for this protocol.
        TooManyOpenFiles, ///< Too many open files / sockets on this process or system.
        SystemNotReady, ///< Underlying socket subsystem not ready (e.g. WSAStartup not called).
        VersionNotSupported, ///< Socket API version not supported.
        Unknown ///< Catch-all for unmapped / unknown errors; inspect getLastErrorCode().
    };
} // namespace reactormq::socket
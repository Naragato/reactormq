//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

// Map build macros to constexpr flags for use with if constexpr in portable code.

namespace reactormq::config
{
#if REACTORMQ_PLATFORM_WINDOWS
    inline constexpr bool kIsWindows = true;
#else
    inline constexpr bool kIsWindows = false;
#endif // REACTORMQ_PLATFORM_WINDOWS

#if REACTORMQ_PLATFORM_LINUX
    inline constexpr bool kIsLinux = true;
#else
    inline constexpr bool kIsLinux = false;
#endif // REACTORMQ_PLATFORM_LINUX

#if REACTORMQ_PLATFORM_ANDROID
    inline constexpr bool kIsAndroid = true;
#else
    inline constexpr bool kIsAndroid = false;
#endif // REACTORMQ_PLATFORM_ANDROID

#if REACTORMQ_PLATFORM_DARWIN_FAMILY
    inline constexpr bool kIsDarwinFamily = true;
#else
    inline constexpr bool kIsDarwinFamily = false;
#endif // REACTORMQ_PLATFORM_DARWIN_FAMILY

#if REACTORMQ_PLATFORM_POSIX_FAMILY
    inline constexpr bool kIsPosixFamily = true;
#else
    inline constexpr bool kIsPosixFamily = false;
#endif // REACTORMQ_PLATFORM_POSIX_FAMILY

#if REACTORMQ_WITH_TLS
    inline constexpr bool kHasOpenSsl = true;
#else
    inline constexpr bool kHasOpenSsl = false;
#endif // REACTORMQ_WITH_TLS

#if REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5
    inline constexpr bool kIsWebsocketUE5 = true;
#else
    inline constexpr bool kIsWebsocketUE5 = false;
#endif // REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5

#if REACTORMQ_WITH_UE5
    inline constexpr bool kIsSocketUE5 = true;
#else
    inline constexpr bool kIsSocketUE5 = false;
#endif // REACTORMQ_WITH_UE5

#if REACTORMQ_WITH_O3DE
    inline constexpr bool kIsO3DESocket = true;
#else
    inline constexpr bool kIsO3DESocket = false;
#endif // REACTORMQ_WITH_O3DE
} // namespace reactormq::config
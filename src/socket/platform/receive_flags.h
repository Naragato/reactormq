//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>

namespace reactormq::socket
{
    /**
     * @brief Flags for socket receive operations.
     * Mapped to platform-specific recv() flags.
     */
    enum class ReceiveFlags : std::uint32_t
    {
        None = 0,
        Peek = 1 << 0, ///< Peek at incoming data without removing it from the queue (MSG_PEEK).
        DontWait = 1 << 1, ///< Non-blocking receive (MSG_DONTWAIT).
        WaitAll = 1 << 2, ///< Wait for full request or error (MSG_WAITALL)
    };

    /**
     * @brief Bitwise OR operator for ReceiveFlags.
     */
    constexpr ReceiveFlags operator|(ReceiveFlags lhs, ReceiveFlags rhs)
    {
        return static_cast<ReceiveFlags>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Bitwise AND operator for ReceiveFlags.
     */
    constexpr ReceiveFlags operator&(ReceiveFlags lhs, ReceiveFlags rhs)
    {
        return static_cast<ReceiveFlags>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Bitwise OR assignment operator for ReceiveFlags.
     */
    constexpr ReceiveFlags& operator|=(ReceiveFlags& lhs, const ReceiveFlags rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    /**
     * @brief Check if a flag is set.
     */
    constexpr bool hasFlag(const ReceiveFlags flags, const ReceiveFlags flag)
    {
        return (flags & flag) == flag;
    }
} // namespace reactormq::socket
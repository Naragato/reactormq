//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <bit>
#include <cstdint>

namespace reactormq::serialize
{
    /**
     * @brief Compile-time flag indicating the host endianness.
     * True when the native byte order is little-endian.
     */
    constexpr bool kIsLittleEndian = std::endian::native == std::endian::little;

    /**
     * @brief Swap byte order of a 16-bit unsigned integer.
     */
    inline uint16_t byteSwapUint16(const uint16_t v)
    {
        return static_cast<uint16_t>(v >> 8 | v << 8);
    }

    /**
     * @brief Swap byte order of a 32-bit unsigned integer.
     */
    inline uint32_t byteSwapUint32(const uint32_t v)
    {
        return v >> 24 | (v >> 8 & 0x0000FF00u) | (v << 8 & 0x00FF0000u) | v << 24;
    }

    /**
     * @brief Swap byte order of a 64-bit unsigned integer.
     */
    inline uint64_t byteSwapUint64(const uint64_t v)
    {
        return (v & 0x00000000000000FFuLL) << 56 | (v & 0x000000000000FF00uLL) << 40 | (v & 0x0000000000FF0000uLL) << 24
            | (v & 0x00000000FF000000uLL) << 8 | (v & 0x000000FF00000000uLL) >> 8 | (v & 0x0000FF0000000000uLL) >> 24
            | (v & 0x00FF000000000000uLL) >> 40 | (v & 0xFF00000000000000uLL) >> 56;
    }

    /**
     * @brief Convert a 16-bit value from host to big-endian order.
     */
    inline uint16_t hostToBigEndian16(const uint16_t v)
    {
        return kIsLittleEndian ? byteSwapUint16(v) : v;
    }

    /**
     * @brief Convert a 32-bit value from host to big-endian order.
     */
    inline uint32_t hostToBigEndian32(const uint32_t v)
    {
        return kIsLittleEndian ? byteSwapUint32(v) : v;
    }

    /**
     * @brief Convert a 64-bit value from host to big-endian order.
     */
    inline uint64_t hostToBigEndian64(const uint64_t v)
    {
        return kIsLittleEndian ? byteSwapUint64(v) : v;
    }

    /**
     * @brief Convert a 16-bit value from big-endian to host order.
     */
    inline uint16_t bigEndianToHost16(const uint16_t v)
    {
        return kIsLittleEndian ? byteSwapUint16(v) : v;
    }

    /**
     * @brief Convert a 32-bit value from big-endian to host order.
     */
    inline uint32_t bigEndianToHost32(const uint32_t v)
    {
        return kIsLittleEndian ? byteSwapUint32(v) : v;
    }

    /**
     * @brief Convert a 64-bit value from big-endian to host order.
     */
    inline uint64_t bigEndianToHost64(const uint64_t v)
    {
        return kIsLittleEndian ? byteSwapUint64(v) : v;
    }
} // namespace reactormq::serialize
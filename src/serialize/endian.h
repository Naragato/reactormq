#pragma once

#include <cstdint>
#include <bit>

namespace reactormq::serialize
{
    constexpr bool kIsLittleEndian = (std::endian::native == std::endian::little);

    inline uint16_t bswap16(uint16_t v)
    {
        return static_cast<uint16_t>((v >> 8) | (v << 8));
    }

    inline uint32_t bswap32(uint32_t v)
    {
        return (v >> 24) | ((v >> 8) & 0x0000FF00u) | ((v << 8) & 0x00FF0000u) | (v << 24);
    }

    inline uint64_t bswap64(uint64_t v)
    {
        return ((v & 0x00000000000000FFull) << 56) |
               ((v & 0x000000000000FF00ull) << 40) |
               ((v & 0x0000000000FF0000ull) << 24) |
               ((v & 0x00000000FF000000ull) << 8) |
               ((v & 0x000000FF00000000ull) >> 8) |
               ((v & 0x0000FF0000000000ull) >> 24) |
               ((v & 0x00FF000000000000ull) >> 40) |
               ((v & 0xFF00000000000000ull) >> 56);
    }

    inline uint16_t htobe16(uint16_t v) { return kIsLittleEndian ? bswap16(v) : v; }
    inline uint32_t htobe32(uint32_t v) { return kIsLittleEndian ? bswap32(v) : v; }
    inline uint64_t htobe64(uint64_t v) { return kIsLittleEndian ? bswap64(v) : v; }
    inline uint16_t be16toh(uint16_t v) { return kIsLittleEndian ? bswap16(v) : v; }
    inline uint32_t be32toh(uint32_t v) { return kIsLittleEndian ? bswap32(v) : v; }
    inline uint64_t be64toh(uint64_t v) { return kIsLittleEndian ? bswap64(v) : v; }
}

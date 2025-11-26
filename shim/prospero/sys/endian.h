//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <stdint.h>

#define _LITTLE_ENDIAN 1234
#define _BIG_ENDIAN 4321
#define _PDP_ENDIAN 3412

#ifndef _BYTE_ORDER
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif

static inline uint16_t _rmq_bswap16(const uint16_t x)
{
    return (uint16_t)(x << 8 | x >> 8);
}

static inline uint32_t _rmq_bswap32(const uint32_t x)
{
    return (x & 0x000000FFu) << 24 | (x & 0x0000FF00u) << 8 | (x & 0x00FF0000u) >> 8 | (x & 0xFF000000u) >> 24;
}

static inline uint64_t _rmq_bswap64(const uint64_t x)
{
    return (x & 0x00000000000000FFull) << 56 | (x & 0x000000000000FF00ull) << 40 | (x & 0x0000000000FF0000ull) << 24
        | (x & 0x00000000FF000000ull) << 8 | (x & 0x000000FF00000000ull) >> 8 | (x & 0x0000FF0000000000ull) >> 24
        | (x & 0x00FF000000000000ull) >> 40 | (x & 0xFF00000000000000ull) >> 56;
}

#if _BYTE_ORDER == _LITTLE_ENDIAN

#define htobe16(x) _rmq_bswap16((uint16_t)(x))
#define htole16(x) ((uint16_t)(x))
#define be16toh(x) _rmq_bswap16((uint16_t)(x))
#define le16toh(x) ((uint16_t)(x))

#define htobe32(x) _rmq_bswap32((uint32_t)(x))
#define htole32(x) ((uint32_t)(x))
#define be32toh(x) _rmq_bswap32((uint32_t)(x))
#define le32toh(x) ((uint32_t)(x))

#define htobe64(x) _rmq_bswap64((uint64_t)(x))
#define htole64(x) ((uint64_t)(x))
#define be64toh(x) _rmq_bswap64((uint64_t)(x))
#define le64toh(x) ((uint64_t)(x))

#else

#define htobe16(x) ((uint16_t)(x))
#define htole16(x) _rmq_bswap16((uint16_t)(x))
#define be16toh(x) ((uint16_t)(x))
#define le16toh(x) _rmq_bswap16((uint16_t)(x))

#define htobe32(x) ((uint32_t)(x))
#define htole32(x) _rmq_bswap32((uint32_t)(x))
#define be32toh(x) ((uint32_t)(x))
#define le32toh(x) _rmq_bswap32((uint32_t)(x))

#define htobe64(x) ((uint64_t)(x))
#define htole64(x) _rmq_bswap64((uint64_t)(x))
#define be64toh(x) ((uint64_t)(x))
#define le64toh(x) _rmq_bswap64((uint64_t)(x))

#endif
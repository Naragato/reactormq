//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/fixed_header.h"
#include "serialize/bytes.h"

#include <cstddef>

using reactormq::mqtt::packets::FixedHeader;
using reactormq::serialize::ByteReader;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size)
{
    if (data == nullptr || size < 2)
    {
        return 0;
    }

    const std::span buffer{ reinterpret_cast<const std::byte*>(data), size };
    ByteReader reader(buffer);

    const FixedHeader header = FixedHeader::create(reader);

    (void)header.getPacketType();
    (void)header.getRemainingLength();
    (void)header.getFlags();

    return 0;
}

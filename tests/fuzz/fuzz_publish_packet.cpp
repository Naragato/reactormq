//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/packet_type.h"
#include "mqtt/packets/publish.h"
#include "reactormq/mqtt/protocol_version.h"
#include "serialize/bytes.h"

#include <cstddef>

using reactormq::mqtt::packets::FixedHeader;
using reactormq::mqtt::packets::PacketType;
using reactormq::mqtt::packets::ProtocolVersion;
using reactormq::mqtt::packets::Publish;
using reactormq::serialize::ByteReader;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size)
{
    if (data == nullptr || size < 4)
    {
        return 0;
    }

    const std::span buffer{ reinterpret_cast<const std::byte*>(data), size };
    ByteReader reader(buffer);

    const FixedHeader header = FixedHeader::create(reader);

    if (header.getPacketType() == PacketType::Publish)
    {
        Publish<ProtocolVersion::V311> packet(reader, header);
        (void)packet.getPayload();
        (void)packet.getTopicName();
    }

    return 0;
}

//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "serialize/bytes.h"

namespace reactormq::mqtt::packets
{
    /**
     * @brief MQTT PINGRESP packet.
     * PINGRESP is sent from server to client in response to a PINGREQ.
     * It has no payload and the remaining length is always 0.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901200
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718086
     */
    class PingResp final : public TControlPacket<PacketType::PingResp>
    {
    public:
        explicit PingResp()
        {
            setFixedHeader(FixedHeader::create(this));
        }

        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        using TControlPacket::TControlPacket;

        /**
         * @brief Encode the packet to a ByteWriter.
         * Only encodes the fixed header (no payload).
         * @param writer ByteWriter to write to.
         */
        void encode(serialize::ByteWriter& writer) const override;

        /**
         * @brief Decode the packet from a ByteReader.
         * PINGRESP has no payload, so this is a no-op.
         * @param reader ByteReader to read from.
         * @return true on success, false on failure.
         */
        bool decode(serialize::ByteReader& reader) override;

        /**
         * @brief Get the length of the packet payload.
         * PINGRESP has no payload, so this always returns 0.
         * @return 0 (no payload).
         */
        [[nodiscard]] uint32_t getLength() const override
        {
            return 0;
        }
    };
} // namespace reactormq::mqtt::packets
#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/packet_type.h"
#include "mqtt/packets/interface/control_packet.h"

#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base for MQTT control packets using CRTP.
     * Provides common fields and helpers shared by specific packet types.
     * @tparam TPacketType PacketType constant for the derived packet.
     */
    template<PacketType TPacketType>
    class TControlPacket : public IControlPacket
    {
    public:
        /// @brief Error message used when a packet fails validation.
        static constexpr auto kInvalidPacket = "Invalid packet";

        explicit TControlPacket() = default;

        /**
         * @brief Construct with a parsed fixed header.
         * @param fixedHeader Fixed header to attach to this packet.
         */
        explicit TControlPacket(const FixedHeader& fixedHeader)
            : m_fixedHeader(fixedHeader)
        {
        }

        ~TControlPacket() override = default;

        /**
         * @brief MQTT control packet type for this instance.
         * @return PacketType value.
         */
        [[nodiscard]] PacketType getPacketType() const override
        {
            return TPacketType;
        }

        /**
         * @brief Whether this packet passed validation.
         * @return True if valid; false otherwise.
         */
        [[nodiscard]] bool isValid() const override
        {
            return m_isValid;
        }

    protected:
        FixedHeader m_fixedHeader;
        bool m_isValid{true};
        static constexpr uint8_t kStringLengthFieldSize = sizeof(uint16_t);
    };
} // namespace reactormq::mqtt::packets

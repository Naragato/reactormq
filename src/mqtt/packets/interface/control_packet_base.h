#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/packet_type.h"
#include "mqtt/packets/interface/control_packet.h"

#include <cstdint>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base class for MQTT control packets using CRTP pattern.
     * 
     * "The MQTT protocol works by exchanging a series of MQTT Control Packets in a defined way."
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901019
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_MQTT_Control_Packets
     * 
     * @tparam TPacketType The specific packet type this class represents.
     */
    template<PacketType TPacketType>
    class TControlPacket : public IControlPacket
    {
    public:
        /// @brief Error message for invalid packet.
        static constexpr const char* kInvalidPacket = "Invalid packet";

        /**
         * @brief Default constructor.
         */
        explicit TControlPacket()
            : m_isValid(true)
        {
        }

        /**
         * @brief Constructor with fixed header.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit TControlPacket(const FixedHeader& fixedHeader)
            : m_fixedHeader(fixedHeader)
              , m_isValid(true)
        {
        }

        /**
         * @brief Virtual destructor.
         */
        ~TControlPacket() override = default;

        /**
         * @brief Get the packet type.
         * @return The packet type for this control packet.
         */
        [[nodiscard]] PacketType getPacketType() const override
        {
            return TPacketType;
        }

        /**
         * @brief Check if this packet is valid.
         * @return True if the packet is valid, false otherwise.
         */
        [[nodiscard]] bool isValid() const override
        {
            return m_isValid;
        }

    protected:
        /// @brief The fixed header for this packet.
        FixedHeader m_fixedHeader;

        /// @brief Whether this packet is valid.
        bool m_isValid;

        /// @brief Size of the string length field (2 bytes for uint16 length prefix).
        static constexpr uint8_t kStringLengthFieldSize = sizeof(uint16_t);
    };
} // namespace reactormq::mqtt::packets
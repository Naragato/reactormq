#include "sub_ack.h"

namespace reactormq::mqtt::packets
{
    // MQTT 3.1.1
    void SubAck<ProtocolVersion::V311>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][SubAck] MQTT 3.1.1");
        m_fixedHeader.encode(writer);
        writer.writeUint16(m_packetIdentifier);

        for (const auto& returnCode : m_returnCodes)
        {
            writer.writeUint8(static_cast<uint8_t>(returnCode));
        }
    }

    bool SubAck<ProtocolVersion::V311>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][SubAck] MQTT 3.1.1");

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            m_isValid = false;
            return false;
        }

        uint32_t payloadSize = m_fixedHeader.getRemainingLength() - 2;

        while (payloadSize > 0)
        {
            uint8_t returnCodeByte = 0;
            if (!reader.tryReadUint8(returnCodeByte))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read return code");
                m_isValid = false;
                return false;
            }

            m_returnCodes.push_back(static_cast<SubscribeReturnCode>(returnCodeByte));
            payloadSize -= 1;
        }

        return true;
    }

    // MQTT 5
    void SubAck<ProtocolVersion::V5>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][SubAck] MQTT 5");
        m_fixedHeader.encode(writer);
        writer.writeUint16(m_packetIdentifier);
        m_properties.encode(writer);

        for (const auto& reasonCode : m_reasonCodes)
        {
            writer.writeUint8(static_cast<uint8_t>(reasonCode));
        }
    }

    bool SubAck<ProtocolVersion::V5>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][SubAck] MQTT 5");

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            m_isValid = false;
            return false;
        }

        m_properties.decode(reader);

        uint32_t payloadSize = m_fixedHeader.getRemainingLength() - 2 - m_properties.getLength();

        while (payloadSize > 0)
        {
            uint8_t reasonCodeByte = 0;
            if (!reader.tryReadUint8(reasonCodeByte))
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read reason code");
                m_isValid = false;
                return false;
            }

            m_reasonCodes.push_back(static_cast<ReasonCode>(reasonCodeByte));
            payloadSize -= 1;
        }

        return true;
    }
} // namespace reactormq::mqtt::packets
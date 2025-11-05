#include "pub_ack.h"

namespace reactormq::mqtt::packets
{
    // MQTT 3.1.1
    void PubAck<ProtocolVersion::V311>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][PubAck] MQTT 3.1.1");
        m_fixedHeader.encode(writer);
        writer.writeUint16(m_packetIdentifier);
    }

    bool PubAck<ProtocolVersion::V311>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][PubAck] MQTT 3.1.1");

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            m_isValid = false;
            return false;
        }

        return true;
    }

    // MQTT 5
    void PubAck<ProtocolVersion::V5>::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][PubAck] MQTT 5");
        m_fixedHeader.encode(writer);
        writer.writeUint16(m_packetIdentifier);

        if (m_reasonCode == ReasonCode::Success && m_properties.getLength(false) == 0)
        {
            return;
        }

        writer.writeUint8(static_cast<uint8_t>(m_reasonCode));

        m_properties.encode(writer);
    }

    bool PubAck<ProtocolVersion::V5>::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][PubAck] MQTT 5");

        if (!reader.tryReadUint16(m_packetIdentifier))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read packet identifier");
            m_isValid = false;
            return false;
        }

        if (reader.getRemaining() == 0)
        {
            m_reasonCode = ReasonCode::Success;
            return true;
        }

        uint8_t reasonCodeByte = 0;
        if (!reader.tryReadUint8(reasonCodeByte))
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to read reason code");
            m_isValid = false;
            return false;
        }
        m_reasonCode = static_cast<ReasonCode>(reasonCodeByte);

        if (reader.getRemaining() > 0)
        {
            m_properties.decode(reader);
        }

        return true;
    }

    const properties::Properties& PubAck<ProtocolVersion::V5>::getProperties() const
    {
        return m_properties;
    }
} // namespace reactormq::mqtt::packets
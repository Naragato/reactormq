#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "serialize/bytes.h"
#include "util/logging/logging.h"

#include <string>
#include <utility>

namespace reactormq::mqtt::packets
{
    /**
     * @brief Base class for MQTT CONNECT packets.
     * 
     * CONNECT is the first packet sent from a client to a server when establishing a connection.
     * 
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901033
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718028
     */
    class ConnectBase : public TControlPacket<PacketType::Connect>
    {
    public:
        /**
         * @brief Constructor from FixedHeader.
         * Used during deserialization.
         * @param fixedHeader The fixed header for this packet.
         */
        explicit ConnectBase(const FixedHeader& fixedHeader)
            : TControlPacket(fixedHeader)
              , m_keepAliveSeconds(60)
              , m_bCleanSession(false)
              , m_bRetainWill(false)
              , m_willQos(QualityOfService::AtMostOnce)
        {
        }

        /**
         * @brief Constructor with all connection parameters.
         * @param clientId The client identifier.
         * @param keepAliveSeconds The keep alive interval in seconds.
         * @param username The username for authentication.
         * @param password The password for authentication.
         * @param bCleanSession Whether to start a clean session.
         * @param bRetainWill Whether to retain the will message.
         * @param willTopic The will topic.
         * @param willMessage The will message payload.
         * @param willQos The will message QoS level.
         */
        explicit ConnectBase(
            std::string clientId,
            const uint16_t keepAliveSeconds,
            std::string username,
            std::string password,
            const bool bCleanSession,
            const bool bRetainWill,
            std::string willTopic,
            std::string willMessage,
            const QualityOfService willQos)
            : TControlPacket()
              , m_clientId(std::move(clientId))
              , m_keepAliveSeconds(keepAliveSeconds)
              , m_username(std::move(username))
              , m_password(std::move(password))
              , m_bCleanSession(bCleanSession)
              , m_bRetainWill(bRetainWill)
              , m_willTopic(std::move(willTopic))
              , m_willMessage(std::move(willMessage))
              , m_willQos(willQos)
        {
        }

        /**
         * @brief Get the client identifier.
         * @return The client identifier.
         */
        [[nodiscard]] const std::string& getClientId() const
        {
            return m_clientId;
        }

        /**
         * @brief Get the keep alive interval in seconds.
         * @return The keep alive interval.
         */
        [[nodiscard]] uint16_t getKeepAliveSeconds() const
        {
            return m_keepAliveSeconds;
        }

        /**
         * @brief Get the username.
         * @return The username.
         */
        [[nodiscard]] const std::string& getUsername() const
        {
            return m_username;
        }

        /**
         * @brief Get the password.
         * @return The password.
         */
        [[nodiscard]] const std::string& getPassword() const
        {
            return m_password;
        }

        /**
         * @brief Get the clean session flag.
         * @return The clean session flag.
         */
        [[nodiscard]] bool getCleanSession() const
        {
            return m_bCleanSession;
        }

        /**
         * @brief Get the retain will flag.
         * @return The retain will flag.
         */
        [[nodiscard]] bool getRetainWill() const
        {
            return m_bRetainWill;
        }

        /**
         * @brief Get the will topic.
         * @return The will topic.
         */
        [[nodiscard]] const std::string& getWillTopic() const
        {
            return m_willTopic;
        }

        /**
         * @brief Get the will message.
         * @return The will message.
         */
        [[nodiscard]] const std::string& getWillMessage() const
        {
            return m_willMessage;
        }

        /**
         * @brief Get the will QoS level.
         * @return The will QoS level.
         */
        [[nodiscard]] QualityOfService getWillQos() const
        {
            return m_willQos;
        }

    protected:
        std::string m_clientId;
        uint16_t m_keepAliveSeconds;
        std::string m_username;
        std::string m_password;
        bool m_bCleanSession;
        bool m_bRetainWill;
        std::string m_willTopic;
        std::string m_willMessage;
        QualityOfService m_willQos;
    };

    template<ProtocolVersion TProtocolVersion>
    class Connect;

    /**
     * @brief MQTT 3.1.1 CONNECT packet.
     * 
     */
    template<>
    class Connect<ProtocolVersion::V311> final : public ConnectBase
    {
    public:
        /**
         * @brief Constructor with connection parameters.
         * @param clientId The client identifier (default: empty).
         * @param keepAliveSeconds The keep alive interval in seconds (default: 60).
         * @param username The username for authentication (default: empty).
         * @param password The password for authentication (default: empty).
         * @param bCleanSession Whether to start a clean session (default: false).
         * @param bRetainWill Whether to retain the will message (default: false).
         * @param willTopic The will topic (default: empty).
         * @param willMessage The will message payload (default: empty).
         * @param willQos The will message QoS level (default: AtMostOnce).
         */
        explicit Connect(
            const std::string& clientId = {},
            const uint16_t keepAliveSeconds = 60,
            const std::string& username = {},
            const std::string& password = {},
            const bool bCleanSession = false,
            const bool bRetainWill = false,
            const std::string& willTopic = {},
            const std::string& willMessage = {},
            const QualityOfService willQos = QualityOfService::AtMostOnce)
            : ConnectBase(
                clientId,
                keepAliveSeconds,
                username,
                password,
                bCleanSession,
                bRetainWill,
                willTopic,
                willMessage,
                willQos)
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Connect(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : ConnectBase(fixedHeader)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Get the length of the packet payload (remaining length).
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override;

        /**
         * @brief Encode the packet to a ByteWriter.
         * @param writer ByteWriter to write to.
         */
        void encode(serialize::ByteWriter& writer) const override;

        /**
         * @brief Decode the packet from a ByteReader.
         * @param reader ByteReader to read from.
         * @return true on success, false on failure.
         */
        bool decode(serialize::ByteReader& reader) override;
    };

    /**
     * @brief MQTT 5 CONNECT packet.
     * 
     */
    template<>
    class Connect<ProtocolVersion::V5> final : public ConnectBase
    {
    public:
        /**
         * @brief Constructor with connection parameters.
         * @param clientId The client identifier (default: empty).
         * @param keepAliveSeconds The keep alive interval in seconds (default: 60).
         * @param username The username for authentication (default: empty).
         * @param password The password for authentication (default: empty).
         * @param bCleanSession Whether to start a clean session (default: false).
         * @param bRetainWill Whether to retain the will message (default: false).
         * @param willTopic The will topic (default: empty).
         * @param willMessage The will message payload (default: empty).
         * @param willQos The will message QoS level (default: AtMostOnce).
         * @param willProperties Properties for the will message (default: empty).
         * @param properties Connect packet properties (default: empty).
         */
        explicit Connect(
            const std::string& clientId = {},
            const uint16_t keepAliveSeconds = 60,
            const std::string& username = {},
            const std::string& password = {},
            const bool bCleanSession = false,
            const bool bRetainWill = false,
            const std::string& willTopic = {},
            const std::string& willMessage = {},
            const QualityOfService willQos = QualityOfService::AtMostOnce,
            properties::Properties willProperties = properties::Properties{},
            properties::Properties properties = properties::Properties{})
            : ConnectBase(
                  clientId,
                  keepAliveSeconds,
                  username,
                  password,
                  bCleanSession,
                  bRetainWill,
                  willTopic,
                  willMessage,
                  willQos)
              , m_willProperties(std::move(willProperties))
              , m_properties(std::move(properties))
        {
            m_fixedHeader = FixedHeader::create(this);
        }

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Connect(serialize::ByteReader& reader, const FixedHeader& fixedHeader)
            : ConnectBase(fixedHeader)
        {
            m_isValid = decode(reader);
        }

        /**
         * @brief Get the length of the packet payload (remaining length).
         * @return The length in bytes.
         */
        [[nodiscard]] uint32_t getLength() const override;

        /**
         * @brief Get the will properties.
         * @return The will properties.
         */
        [[nodiscard]] const properties::Properties& getWillProperties() const
        {
            return m_willProperties;
        }

        /**
         * @brief Get the connect properties.
         * @return The connect properties.
         */
        [[nodiscard]] const properties::Properties& getProperties() const
        {
            return m_properties;
        }

        /**
         * @brief Encode the packet to a ByteWriter.
         * @param writer ByteWriter to write to.
         */
        void encode(serialize::ByteWriter& writer) const override;

        /**
         * @brief Decode the packet from a ByteReader.
         * @param reader ByteReader to read from.
         * @return true on success, false on failure.
         */
        bool decode(serialize::ByteReader& reader) override;

    protected:
        properties::Properties m_willProperties;
        properties::Properties m_properties;
    };

    using Connect3 = Connect<ProtocolVersion::V311>;
    using Connect5 = Connect<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets

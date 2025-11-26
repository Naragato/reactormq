//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/fixed_header.h"
#include "mqtt/packets/interface/control_packet_base.h"
#include "mqtt/packets/properties/properties.h"
#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "serialize/bytes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace reactormq::mqtt::packets
{
    namespace detail
    {
        /**
         * @brief Traits for MQTT CONNECT packets.
         * @tparam V Protocol version.
         */
        template<ProtocolVersion V>
        struct ConnectTraits;

        /**
         * @brief Traits specialization for MQTT 3.1.1 CONNECT packets.
         */
        template<>
        struct ConnectTraits<ProtocolVersion::V311>
        {
            static constexpr bool HasProperties = false;
            using PropertiesType = properties::EmptyProperties;
            using WillPropertiesType = properties::EmptyProperties;
        };

        /**
         * @brief Traits specialization for MQTT 5 CONNECT packets.
         */
        template<>
        struct ConnectTraits<ProtocolVersion::V5>
        {
            static constexpr bool HasProperties = true;
            using PropertiesType = properties::Properties;
            using WillPropertiesType = properties::Properties;
        };
    } // namespace detail

    /**
     * @brief Interface for MQTT CONNECT packets.
     */
    class IConnectPacket : public TControlPacket<PacketType::Connect>
    {
    public:
        /**
         * @brief Default constructor.
         */
        IConnectPacket() = default;

        /**
         * @brief Constructor from FixedHeader.
         * @param fixedHeader The fixed header for this packet.
         */
        using TControlPacket::TControlPacket;

        /**
         * @brief Virtual destructor.
         */
        ~IConnectPacket() override = default;

        /**
         * @brief Get the client identifier.
         * @return The client identifier.
         */
        [[nodiscard]] virtual const std::string& getClientId() const = 0;

        /**
         * @brief Get the keep alive interval in seconds.
         * @return The keep alive interval.
         */
        [[nodiscard]] virtual std::uint16_t getKeepAliveSeconds() const = 0;

        /**
         * @brief Get the username.
         * @return The username.
         */
        [[nodiscard]] virtual const std::string& getUsername() const = 0;

        /**
         * @brief Get the password.
         * @return The password.
         */
        [[nodiscard]] virtual const std::string& getPassword() const = 0;

        /**
         * @brief Get the clean session flag.
         * @return The clean session flag.
         */
        [[nodiscard]] virtual bool getCleanSession() const = 0;

        /**
         * @brief Get the retain will flag.
         * @return The retain will flag.
         */
        [[nodiscard]] virtual bool getRetainWill() const = 0;

        /**
         * @brief Get the will topic.
         * @return The will topic.
         */
        [[nodiscard]] virtual const std::string& getWillTopic() const = 0;

        /**
         * @brief Get the will message.
         * @return The will message.
         */
        [[nodiscard]] virtual const std::string& getWillMessage() const = 0;

        /**
         * @brief Get the will QoS level.
         * @return The will QoS level.
         */
        [[nodiscard]] virtual QualityOfService getWillQos() const = 0;
    };

    /**
     * @brief MQTT CONNECT packet for a specific protocol version.
     *
     * CONNECT is the first packet sent from a client to a server when establishing a connection.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901033
     * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718028
     *
     * @tparam TProtocolVersion Protocol version.
     */
    template<ProtocolVersion TProtocolVersion>
    class Connect final : public IConnectPacket
    {
        using Traits = detail::ConnectTraits<TProtocolVersion>;
        using PropertiesStorage = typename Traits::PropertiesType;
        using WillPropertiesStorage = typename Traits::WillPropertiesType;

    public:
        /**
         * @brief Constructor with connection parameters.
         * @param clientId The client identifier.
         * @param keepAliveSeconds The keep alive interval in seconds.
         * @param username The username for authentication.
         * @param password The password for authentication.
         * @param cleanSession Whether to start a clean session.
         * @param retainWill Whether to retain the will message.
         * @param willTopic The will topic.
         * @param willMessage The will message payload.
         * @param willQos The will message QoS level.
         */
        explicit Connect(
            std::string clientId = {},
            uint16_t keepAliveSeconds = 60,
            std::string username = {},
            std::string password = {},
            bool cleanSession = false,
            bool retainWill = false,
            std::string willTopic = {},
            std::string willMessage = {},
            QualityOfService willQos = QualityOfService::AtMostOnce);

        /**
         * @brief Constructor with connection parameters and properties (MQTT 5 only).
         * @param clientId The client identifier.
         * @param keepAliveSeconds The keep alive interval in seconds.
         * @param username The username for authentication.
         * @param password The password for authentication.
         * @param cleanSession Whether to start a clean session.
         * @param retainWill Whether to retain the will message.
         * @param willTopic The will topic.
         * @param willMessage The will message payload.
         * @param willQos The will message QoS level.
         * @param willProperties Properties for the will message.
         * @param properties CONNECT packet properties.
         */
        explicit Connect(
            std::string clientId,
            uint16_t keepAliveSeconds,
            std::string username,
            std::string password,
            bool cleanSession,
            bool retainWill,
            std::string willTopic,
            std::string willMessage,
            QualityOfService willQos,
            typename Traits::WillPropertiesType willProperties,
            typename Traits::PropertiesType properties)
            requires(detail::ConnectTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Constructor for deserialization.
         * @param reader Reader for deserialization.
         * @param fixedHeader The fixed header.
         */
        explicit Connect(serialize::ByteReader& reader, const FixedHeader& fixedHeader);

        /**
         * @brief Get the client identifier.
         * @return The client identifier.
         */
        [[nodiscard]] const std::string& getClientId() const override;

        /**
         * @brief Get the keep alive interval in seconds.
         * @return The keep alive interval.
         */
        [[nodiscard]] uint16_t getKeepAliveSeconds() const override;

        /**
         * @brief Get the username.
         * @return The username.
         */
        [[nodiscard]] const std::string& getUsername() const override;

        /**
         * @brief Get the password.
         * @return The password.
         */
        [[nodiscard]] const std::string& getPassword() const override;

        /**
         * @brief Get the clean session flag.
         * @return The clean session flag.
         */
        [[nodiscard]] bool getCleanSession() const override;

        /**
         * @brief Get the retain will flag.
         * @return The retain will flag.
         */
        [[nodiscard]] bool getRetainWill() const override;

        /**
         * @brief Get the will topic.
         * @return The will topic.
         */
        [[nodiscard]] const std::string& getWillTopic() const override;

        /**
         * @brief Get the will message.
         * @return The will message.
         */
        [[nodiscard]] const std::string& getWillMessage() const override;

        /**
         * @brief Get the will QoS level.
         * @return The will QoS level.
         */
        [[nodiscard]] QualityOfService getWillQos() const override;

        /**
         * @brief Get the will properties (MQTT 5 only).
         * @return The will properties.
         */
        [[nodiscard]] const typename Traits::WillPropertiesType& getWillProperties() const
            requires(detail::ConnectTraits<TProtocolVersion>::HasProperties);

        /**
         * @brief Get the CONNECT properties (MQTT 5 only).
         * @return The CONNECT properties.
         */
        [[nodiscard]] const typename Traits::PropertiesType& getProperties() const
            requires(detail::ConnectTraits<TProtocolVersion>::HasProperties);

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

    private:
        std::string m_clientId;
        uint16_t m_keepAliveSeconds{};
        std::string m_username;
        std::string m_password;
        bool m_cleanSession{};
        bool m_retainWill{};
        std::string m_willTopic;
        std::string m_willMessage;
        QualityOfService m_willQos{ QualityOfService::AtMostOnce };
        WillPropertiesStorage m_willProperties{};
        PropertiesStorage m_properties{};

        static constexpr std::byte kCleanSessionBit{ std::byte{ 0x1 } << 1 };
        static constexpr std::byte kWillFlagBit{ std::byte{ 0x1 } << 2 };
        static constexpr std::byte kWillRetainBit{ std::byte{ 0x1 } << 5 };
        static constexpr std::byte kPasswordBit{ std::byte{ 0x1 } << 6 };
        static constexpr std::byte kUsernameBit{ std::byte{ 0x1 } << 7 };

        static constexpr std::byte kWillQosMask{ std::byte{ 0x3 } };
        static constexpr int kWillQosShift{ 3 };
    };

    /**
     * @brief Encode a CONNECT packet to the writer.
     * @tparam V Protocol version.
     * @param writer Writer to encode to.
     * @param clientId Client identifier.
     * @param keepAliveSeconds Keep alive interval.
     * @param username Username.
     * @param password Password.
     * @param cleanSession Clean session flag.
     * @param authMethod Authentication method (MQTT 5 only).
     * @param initialAuthData Initial authentication data (MQTT 5 only).
     */
    template<ProtocolVersion V>
    void encodeConnectToWriter(
        serialize::ByteWriter& writer,
        const std::string& clientId,
        std::uint16_t keepAliveSeconds,
        const std::string& username,
        const std::string& password,
        bool cleanSession,
        const std::string& authMethod,
        const std::vector<std::uint8_t>& initialAuthData);

    /**
     * @brief Alias for MQTT 3.1.1 CONNECT packet.
     */
    using Connect3 = Connect<ProtocolVersion::V311>;

    /**
     * @brief Alias for MQTT 5 CONNECT packet.
     */
    using Connect5 = Connect<ProtocolVersion::V5>;
} // namespace reactormq::mqtt::packets
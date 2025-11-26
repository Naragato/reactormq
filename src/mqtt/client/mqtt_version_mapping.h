//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/conn_ack.h"
#include "mqtt/packets/connect.h"
#include "mqtt/packets/disconnect.h"
#include "mqtt/packets/pub_ack.h"
#include "mqtt/packets/pub_comp.h"
#include "mqtt/packets/pub_rec.h"
#include "mqtt/packets/pub_rel.h"
#include "mqtt/packets/publish.h"
#include "mqtt/packets/sub_ack.h"
#include "mqtt/packets/subscribe.h"
#include "mqtt/packets/unsub_ack.h"
#include "mqtt/packets/unsubscribe.h"
#include "reactormq/mqtt/protocol_version.h"

namespace reactormq::mqtt
{
    struct Message;
}

namespace reactormq::mqtt::client
{
    /**
     * @brief Parse an MQTT control packet for a specific protocol version.
     *
     * This function is instantiated per MQTT protocol version and is responsible
     * for constructing the appropriate concrete control packet type from the
     * provided fixed header and reader.
     *
     * @tparam V MQTT protocol version (e.g. packets::ProtocolVersion::V311 or V5).
     * @param reader      Byte reader positioned at the start of the packet body.
     * @param fixedHeader Fixed header already parsed for this packet.
     * @param packetType  MQTT packet type encoded in the fixed header.
     * @return A unique_ptr to the parsed control packet, or nullptr on failure.
     */
    template<packets::ProtocolVersion V>
    std::unique_ptr<packets::IControlPacket> parsePacketImpl(
        serialize::ByteReader& reader, const packets::FixedHeader& fixedHeader, packets::PacketType packetType);

    /**
     * @brief Compile-time mapping between an MQTT protocol version and its packet types.
     *
     * This template provides type aliases for all MQTT control packet types
     * for a given protocol version, as well as version-specific capabilities
     * (e.g. whether the version supports AUTH).
     *
     * Usage:
     * @code
     * using Mapping = MqttVersionMapping<packets::ProtocolVersion::V5>;
     * using PublishPacket = Mapping::Publish;
     * @endcode
     *
     * @tparam V MQTT protocol version.
     */
    template<packets::ProtocolVersion V>
    struct MqttVersionMapping;

    /**
     * @brief MQTT 3.1.1 version mapping.
     *
     * Provides the concrete packet types for MQTT 3.1.1 and indicates
     * that this version does not support the AUTH packet.
     */
    template<>
    struct MqttVersionMapping<packets::ProtocolVersion::V311>
    {
        using Connect = packets::Connect<packets::ProtocolVersion::V311>;
        using ConnAck = packets::ConnAck<packets::ProtocolVersion::V311>;
        using Publish = packets::Publish<packets::ProtocolVersion::V311>;
        using PubAck = packets::PubAck<packets::ProtocolVersion::V311>;
        using PubRec = packets::PubRec<packets::ProtocolVersion::V311>;
        using PubRel = packets::PubRel<packets::ProtocolVersion::V311>;
        using PubComp = packets::PubComp<packets::ProtocolVersion::V311>;
        using Subscribe = packets::Subscribe<packets::ProtocolVersion::V311>;
        using SubAck = packets::SubAck<packets::ProtocolVersion::V311>;
        using Unsubscribe = packets::Unsubscribe<packets::ProtocolVersion::V311>;
        using UnsubAck = packets::UnsubAck<packets::ProtocolVersion::V311>;
        using Disconnect = packets::Disconnect<packets::ProtocolVersion::V311>;

        /**
         * @brief Indicates whether this protocol version supports the AUTH packet.
         * @return Always false for MQTT 3.1.1.
         */
        static constexpr bool supportsAuth()
        {
            return false;
        }
    };

    /**
     * @brief MQTT 5.0 version mapping.
     *
     * Provides the concrete packet types for MQTT 5.0 and indicates
     * that this version supports the AUTH packet.
     */
    template<>
    struct MqttVersionMapping<packets::ProtocolVersion::V5>
    {
        using Connect = packets::Connect<packets::ProtocolVersion::V5>;
        using ConnAck = packets::ConnAck<packets::ProtocolVersion::V5>;
        using Publish = packets::Publish<packets::ProtocolVersion::V5>;
        using PubAck = packets::PubAck<packets::ProtocolVersion::V5>;
        using PubRec = packets::PubRec<packets::ProtocolVersion::V5>;
        using PubRel = packets::PubRel<packets::ProtocolVersion::V5>;
        using PubComp = packets::PubComp<packets::ProtocolVersion::V5>;
        using Subscribe = packets::Subscribe<packets::ProtocolVersion::V5>;
        using SubAck = packets::SubAck<packets::ProtocolVersion::V5>;
        using Unsubscribe = packets::Unsubscribe<packets::ProtocolVersion::V5>;
        using UnsubAck = packets::UnsubAck<packets::ProtocolVersion::V5>;
        using Disconnect = packets::Disconnect<packets::ProtocolVersion::V5>;

        /**
         * @brief Indicates whether this protocol version supports the AUTH packet.
         * @return Always true for MQTT 5.0.
         */
        static constexpr bool supportsAuth()
        {
            return true;
        }
    };

    /**
     * @brief Invoke a callable with a compile-time MQTT version tag based on a runtime version value.
     *
     * This helper converts a runtime @c packets::ProtocolVersion into a compile-time
     * integral constant and passes it to the provided callable. It is typically used
     * to select version-specific code paths while keeping the main logic shared.
     *
     * Example:
     * @code
     * withMqttVersion(protocolVersion, [](auto versionTag)
     * {
     *     constexpr auto V = decltype(versionTag)::value;
     *     using Mapping = MqttVersionMapping<V>;
     *     using PublishPacket = typename Mapping::Publish;
     *     ...
     * });
     * @endcode
     *
     * @tparam F Type of the callable.
     * @param version Runtime MQTT protocol version to dispatch on.
     * @param f       Callable to invoke; it must accept a single argument of type
     *                @c std::integral_constant<packets::ProtocolVersion, V>.
     * @return Whatever @p f returns.
     */
    template<typename F>
    decltype(auto) withMqttVersion(const packets::ProtocolVersion version, F&& f)
    {
        switch (version)
        {
        case packets::ProtocolVersion::V311:
            {
                return std::forward<F>(f)(std::integral_constant<packets::ProtocolVersion, packets::ProtocolVersion::V311>{});
            }
        case packets::ProtocolVersion::V5:
            {
                return std::forward<F>(f)(std::integral_constant<packets::ProtocolVersion, packets::ProtocolVersion::V5>{});
            }
        }

        std::terminate();
    }

    template<auto Version>
    struct PublishEncoder;

    template<>
    struct PublishEncoder<packets::ProtocolVersion::V311>
    {
        static void encode(Message const& message, std::uint16_t packetId, serialize::ByteWriter& writer);
    };

    template<>
    struct PublishEncoder<packets::ProtocolVersion::V5>
    {
        static void encode(Message const& message, std::uint16_t packetId, serialize::ByteWriter& writer);
    };
    ;
} // namespace reactormq::mqtt::client
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/mqtt/protocol_version.h"

#include <optional>

namespace reactormq::mqtt
{
    class TopicFilter;
}

namespace reactormq::mqtt::packets
{
    class IControlPacket;
}

namespace reactormq::mqtt::client
{
    struct SubscribeCommand;
    struct SubscribesCommand;
} // namespace reactormq::mqtt::client

namespace reactormq::mqtt::client::acknowledgement::subscription
{
    /**
     * @brief Processes a SUBACK packet and resolves subscription promise.
     * @param packet Received SUBACK packet.
     * @param singleSubscription Optional single subscription command.
     * @param multiSubscription Optional multiple subscriptions command.
     * @param protocolVersion MQTT protocol version.
     */
    void resolve(
        const packets::IControlPacket& packet,
        std::optional<SubscribeCommand>& singleSubscription,
        std::optional<SubscribesCommand>& multiSubscription,
        packets::ProtocolVersion protocolVersion);
} // namespace reactormq::mqtt::client::acknowledgement::subscription
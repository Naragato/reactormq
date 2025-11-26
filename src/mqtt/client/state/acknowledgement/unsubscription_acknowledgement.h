//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/mqtt/protocol_version.h"
#include "reactormq/mqtt/unsubscribe_result.h"

#include <vector>

namespace reactormq::mqtt::packets
{
    class IControlPacket;
}

namespace reactormq::mqtt::client
{
    struct UnsubscribesCommand;
}

namespace reactormq::mqtt::client::acknowledgement::unsubscription
{
    /**
     * @brief Processes an UNSUBACK packet and resolves unsubscription promise.
     * @param packet Received UNSUBACK packet.
     * @param unsubscription Unsubscription command to resolve.
     * @param protocolVersion MQTT protocol version.
     */
    void resolve(const packets::IControlPacket& packet, UnsubscribesCommand& unsubscription, packets::ProtocolVersion protocolVersion);
} // namespace reactormq::mqtt::client::acknowledgement::unsubscription
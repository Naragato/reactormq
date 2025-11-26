//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/client/state/state.h"

namespace reactormq::mqtt::packets
{
    class IPublishPacket;
}

namespace reactormq::mqtt::client
{
    class Context;
}

namespace reactormq::mqtt::client::incoming::publish
{
    /**
     * @brief Handle an incoming PUBLISH packet and broadcasts to subscribers.
     * @param context The client context.
     * @param packet The generic control packet (must be PUBLISH).
     * @return StateTransition (usually noTransition).
     */
    StateTransition broadcast(Context& context, const packets::IControlPacket& packet);
} // namespace reactormq::mqtt::client::incoming::publish
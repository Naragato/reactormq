//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/client/state/state.h"

#include <future>
#include <optional>

namespace reactormq::mqtt::packets
{
    class IControlPacket;
}

namespace reactormq::mqtt::client
{
    class Context;
}

namespace reactormq::mqtt::client::processing::authentication
{
    /**
     * @brief Process an AUTH packet.
     * @param context The client context.
     * @param packet The AUTH packet.
     * @param promise The connection promise to fail if auth fails.
     * @return StateTransition (usually noTransition or to DisconnectedState).
     */
    [[nodiscard]] StateTransition handle(
        const Context& context, const packets::IControlPacket& packet, std::optional<std::promise<Result<void>>>& promise);
} // namespace reactormq::mqtt::client::processing::authentication
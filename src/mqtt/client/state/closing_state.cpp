//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "closing_state.h"
#include "disconnected_state.h"

#include "mqtt/client/context.h"
#include "mqtt/packets/disconnect.h"
#include "serialize/bytes.h"
#include "socket/socket.h"

#include <chrono>
#include <mqtt/client/mqtt_version_mapping.h>

namespace reactormq::mqtt::client
{
    ClosingState::ClosingState(std::promise<Result<void>> promise)
        : m_promise(std::move(promise))
        , m_entryTime(std::chrono::steady_clock::now())
    {
    }

    StateTransition ClosingState::onEnter(Context& context)
    {
        const auto sock = context.getSocket();
        if (sock)
        {
            std::vector<std::byte> buffer;
            serialize::ByteWriter writer(buffer);

            withMqttVersion(
                context.getProtocolVersion(),
                [&writer]<typename VersionTag>(VersionTag)
                {
                    constexpr auto kV = VersionTag::value;
                    packets::encodeDisconnectToWriter<kV>(writer);
                });

            sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
        }

        if (sock)
        {
            sock->disconnect();
        }

        return StateTransition::noTransition();
    }

    void ClosingState::onExit(Context& context)
    {
        if (m_promise.has_value())
        {
            m_promise.value().set_value(Result<void>::success());
            m_promise.reset();
        }

        context.invokeCallback(
            [&ctx = context]
            {
                ctx.getOnDisconnect().broadcast(true);
            });
    }

    StateTransition ClosingState::handleCommand(Context& context, Command& command)
    {
        if (std::holds_alternative<CloseSocketCommand>(command))
        {
            if (const auto sock = context.getSocket())
            {
                sock->disconnect();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(true));
        }

        if (std::holds_alternative<ConnectCommand>(command))
        {
            auto& [cleanSession, promise] = std::get<ConnectCommand>(command);
            promise.set_value(Result<void>::failure("Cannot connect while closing"));
        }
        else if (std::holds_alternative<PublishCommand>(command))
        {
            auto& [message, promise] = std::get<PublishCommand>(command);
            promise.set_value(Result<void>::failure("Cannot publish while closing"));
        }
        else if (std::holds_alternative<SubscribeCommand>(command))
        {
            auto& [topicFilter, promise] = std::get<SubscribeCommand>(command);
            promise.set_value(Result<SubscribeResult>::failure("Cannot subscribe while closing"));
        }
        else if (std::holds_alternative<SubscribesCommand>(command))
        {
            auto& [topicFilters, promise] = std::get<SubscribesCommand>(command);
            promise.set_value(Result<std::vector<SubscribeResult>>::failure("Cannot subscribe while closing"));
        }
        else if (std::holds_alternative<UnsubscribesCommand>(command))
        {
            auto& [topics, promise] = std::get<UnsubscribesCommand>(command);
            promise.set_value(Result<std::vector<UnsubscribeResult>>::failure("Cannot unsubscribe while closing"));
        }
        else if (std::holds_alternative<DisconnectCommand>(command))
        {
            auto& [promise] = std::get<DisconnectCommand>(command);
            promise.set_value(Result<void>::success());
        }

        return StateTransition::noTransition();
    }

    StateTransition ClosingState::onSocketConnected(Context& /*context*/)
    {
        return StateTransition::noTransition();
    }

    StateTransition ClosingState::onSocketDisconnected(Context& /*context*/)
    {
        return StateTransition::transitionTo(std::make_unique<DisconnectedState>(true));
    }

    StateTransition ClosingState::onDataReceived(Context& /*context*/, const uint8_t* /*data*/, const uint32_t /*size*/)
    {
        return StateTransition::noTransition();
    }

    StateTransition ClosingState::onTick(Context& context)
    {
        constexpr std::chrono::milliseconds kCloseTimeout{ 5000 };

        if (const auto elapsed = std::chrono::steady_clock::now() - m_entryTime; elapsed >= kCloseTimeout)
        {
            if (const auto sock = context.getSocket())
            {
                sock->disconnect();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(true));
        }

        return StateTransition::noTransition();
    }
} // namespace reactormq::mqtt::client
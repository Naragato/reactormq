//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "disconnected_state.h"
#include "connecting_state.h"
#include "reactormq/mqtt/result.h"

#include <variant>

namespace reactormq::mqtt::client
{
    DisconnectedState::DisconnectedState(const bool wasGracefulDisconnect)
        : m_wasGracefulDisconnect(wasGracefulDisconnect)
    {
    }

    StateTransition DisconnectedState::onEnter(Context& context)
    {
        if (context.getSocket())
        {
            context.setSocket(nullptr);
        }

        if (const auto settings = context.getSettings(); settings && settings->isAutoReconnectEnabled() && !m_wasGracefulDisconnect)
        {
            m_backoffCalculator.emplace(
                settings->getAutoReconnectInitialDelayMs(),
                settings->getAutoReconnectMaxDelayMs(),
                settings->getAutoReconnectMultiplier());

            const auto delayMs = m_backoffCalculator->calculateNextDelay();
            m_nextRetryTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs);
        }

        return StateTransition::noTransition();
    }

    void DisconnectedState::onExit(Context& /*context*/)
    {
        // Nothing to do
    }

    StateTransition DisconnectedState::handleCommand(Context& /*context*/, Command& command)
    {
        if (std::holds_alternative<ConnectCommand>(command))
        {
            auto& [cleanSession, promise] = std::get<ConnectCommand>(command);

            m_nextRetryTime.reset();
            m_backoffCalculator.reset();

            return StateTransition::transitionTo(std::make_unique<ConnectingState>(cleanSession, std::move(promise)));
        }

        if (std::holds_alternative<PublishCommand>(command))
        {
            auto& [message, promise] = std::get<PublishCommand>(command);
            promise.set_value(Result<void>::failure("Not connected"));
        }
        else if (std::holds_alternative<SubscribesCommand>(command))
        {
            auto& [topicFilters, promise] = std::get<SubscribesCommand>(command);
            promise.set_value(Result<std::vector<SubscribeResult>>::failure("Not connected"));
        }
        else if (std::holds_alternative<SubscribeCommand>(command))
        {
            auto& [topicFilter, promise] = std::get<SubscribeCommand>(command);
            promise.set_value(Result<SubscribeResult>::failure("Not connected"));
        }
        else if (std::holds_alternative<UnsubscribesCommand>(command))
        {
            auto& [topics, promise] = std::get<UnsubscribesCommand>(command);
            promise.set_value(Result<std::vector<UnsubscribeResult>>::failure("Not connected"));
        }
        else if (std::holds_alternative<UnsubscribeCommand>(command))
        {
            auto& [topic, promise] = std::get<UnsubscribeCommand>(command);
            promise.set_value(Result<UnsubscribeResult>::failure("Not connected"));
        }
        else if (std::holds_alternative<DisconnectCommand>(command))
        {
            auto& [promise] = std::get<DisconnectCommand>(command);
            promise.set_value(Result<void>::success());
        }

        return StateTransition::noTransition();
    }

    StateTransition DisconnectedState::onSocketConnected(Context& /*context*/)
    {
        return StateTransition::noTransition();
    }

    StateTransition DisconnectedState::onSocketDisconnected(Context& /*context*/)
    {
        return StateTransition::noTransition();
    }

    StateTransition DisconnectedState::onDataReceived(Context& /*context*/, const uint8_t* /*data*/, uint32_t /*size*/)
    {
        return StateTransition::noTransition();
    }

    StateTransition DisconnectedState::onTick(Context& context)
    {
        if (m_nextRetryTime.has_value())
        {
            const auto now = std::chrono::steady_clock::now();
            if (now >= m_nextRetryTime.value())
            {
                std::promise<Result<void>> promise;
                auto future = promise.get_future();

                const auto settings = context.getSettings();
                const bool cleanSession = settings ? settings->getSessionExpiryInterval() == 0 : true;

                return StateTransition::transitionTo(std::make_unique<ConnectingState>(cleanSession, std::move(promise)));
            }
        }

        return StateTransition::noTransition();
    }
} // namespace reactormq::mqtt::client
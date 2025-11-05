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

        const auto settings = context.getSettings();
        if (settings && settings->isAutoReconnectEnabled() && !m_wasGracefulDisconnect)
        {
            m_backoffCalculator.emplace(
                settings->getAutoReconnectInitialDelayMs(),
                settings->getAutoReconnectMaxDelayMs(),
                settings->getAutoReconnectMultiplier()
                );

            const auto delayMs = m_backoffCalculator->calculateNextDelay();
            m_nextRetryTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs);
        }

        return StateTransition::noTransition();
    }

    void DisconnectedState::onExit(Context& context)
    {
    }

    StateTransition DisconnectedState::handleCommand(Context& context, Command& command)
    {
        if (std::holds_alternative<ConnectCommand>(command))
        {
            auto& connectCmd = std::get<ConnectCommand>(command);

            m_nextRetryTime.reset();
            m_backoffCalculator.reset();

            return StateTransition::transitionTo(
                std::make_unique<ConnectingState>(connectCmd.cleanSession, std::move(connectCmd.promise))
                );
        }

        if (std::holds_alternative<PublishCommand>(command))
        {
            auto& publishCmd = std::get<PublishCommand>(command);
            publishCmd.promise.set_value(Result<void>::failure("Not connected"));
        }
        else if (std::holds_alternative<SubscribesCommand>(command))
        {
            auto& subscribeCmd = std::get<SubscribesCommand>(command);
            subscribeCmd.promise.set_value(Result<std::vector<SubscribeResult>>::failure("Not connected"));
        }
        else if (std::holds_alternative<SubscribeCommand>(command))
        {
            auto& subscribeCmd = std::get<SubscribeCommand>(command);
            subscribeCmd.promise.set_value(Result<SubscribeResult>::failure("Not connected"));
        }
        else if (std::holds_alternative<UnsubscribesCommand>(command))
        {
            auto& unsubscribeCmd = std::get<UnsubscribesCommand>(command);
            unsubscribeCmd.promise.set_value(Result<std::vector<UnsubscribeResult>>::failure("Not connected"));
        }
        else if (std::holds_alternative<UnsubscribeCommand>(command))
        {
            auto& unsubscribeCmd = std::get<UnsubscribeCommand>(command);
            unsubscribeCmd.promise.set_value(Result<UnsubscribeResult>::failure("Not connected"));
        }
        else if (std::holds_alternative<DisconnectCommand>(command))
        {
            auto& disconnectCmd = std::get<DisconnectCommand>(command);
            disconnectCmd.promise.set_value(Result<void>::success());
        }

        return StateTransition::noTransition();
    }

    StateTransition DisconnectedState::onSocketConnected(Context& context)
    {
        return StateTransition::noTransition();
    }

    StateTransition DisconnectedState::onSocketDisconnected(Context& context)
    {
        return StateTransition::noTransition();
    }

    StateTransition DisconnectedState::onDataReceived(Context& context, const uint8_t* data, uint32_t size)
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
                const bool cleanSession = settings ? (settings->getSessionExpiryInterval() == 0) : true;

                return StateTransition::transitionTo(
                    std::make_unique<ConnectingState>(cleanSession, std::move(promise))
                    );
            }
        }

        return StateTransition::noTransition();
    }
} // namespace reactormq::mqtt::client
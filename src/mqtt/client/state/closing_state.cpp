#include "closing_state.h"
#include "disconnected_state.h"

#include "mqtt/client/context.h"
#include "mqtt/packets/disconnect.h"
#include "serialize/bytes.h"
#include "socket/socket_base.h"

#include <chrono>

namespace reactormq::mqtt::client
{
    ClosingState::ClosingState(std::promise<Result<void>> promise)
        : m_promise(std::move(promise)), m_entryTime(std::chrono::steady_clock::now())
    {
    }

    StateTransition ClosingState::onEnter(Context& context)
    {
        const auto sock = context.getSocket();
        if (sock)
        {
            std::vector<std::byte> buffer;
            serialize::ByteWriter writer(buffer);

            const auto protocolVersion = context.getProtocolVersion();
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                const packets::Disconnect<packets::ProtocolVersion::V5> disconnectPacket(
                    ReasonCode::NormalDisconnection,
                    {}
                    );
                disconnectPacket.encode(writer);
            }
            else
            {
                const packets::Disconnect<packets::ProtocolVersion::V311> disconnectPacket;
                disconnectPacket.encode(writer);
            }

            sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));
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
            [&ctx = context]()
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
            auto& cmd = std::get<ConnectCommand>(command);
            cmd.promise.set_value(Result<void>::failure("Cannot connect while closing"));
        }
        else if (std::holds_alternative<PublishCommand>(command))
        {
            auto& cmd = std::get<PublishCommand>(command);
            cmd.promise.set_value(Result<void>::failure("Cannot publish while closing"));
        }
        else if (std::holds_alternative<SubscribeCommand>(command))
        {
            auto& cmd = std::get<SubscribeCommand>(command);
            cmd.promise.set_value(Result<SubscribeResult>::failure("Cannot subscribe while closing"));
        }
        else if (std::holds_alternative<SubscribesCommand>(command))
        {
            auto& cmd = std::get<SubscribesCommand>(command);
            cmd.promise.set_value(Result<std::vector<SubscribeResult>>::failure("Cannot subscribe while closing"));
        }
        else if (std::holds_alternative<UnsubscribesCommand>(command))
        {
            auto& cmd = std::get<UnsubscribesCommand>(command);
            cmd.promise.set_value(Result<std::vector<UnsubscribeResult>>::failure("Cannot unsubscribe while closing"));
        }
        else if (std::holds_alternative<DisconnectCommand>(command))
        {
            auto& cmd = std::get<DisconnectCommand>(command);
            cmd.promise.set_value(Result<void>::success());
        }

        return StateTransition::noTransition();
    }

    StateTransition ClosingState::onSocketConnected(Context& context)
    {
        return StateTransition::noTransition();
    }

    StateTransition ClosingState::onSocketDisconnected(Context& context)
    {
        return StateTransition::transitionTo(std::make_unique<DisconnectedState>(true));
    }

    StateTransition ClosingState::onDataReceived(Context& context, const uint8_t* data, const uint32_t size)
    {
        return StateTransition::noTransition();
    }

    StateTransition ClosingState::onTick(Context& context)
    {
        constexpr std::chrono::milliseconds kCloseTimeout{ 5000 };
        const auto elapsed = std::chrono::steady_clock::now() - m_entryTime;

        if (elapsed >= kCloseTimeout)
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
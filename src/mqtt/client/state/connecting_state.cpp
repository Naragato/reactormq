//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "connecting_state.h"
#include "disconnected_state.h"
#include "mqtt/packets/auth.h"
#include "mqtt/packets/conn_ack.h"
#include "mqtt/packets/connect.h"
#include "mqtt/packets/fixed_header.h"
#include "processing/authentication_handler.h"
#include "ready_state.h"
#include "serialize/bytes.h"
#include "socket/socket.h"

#include <cstring>
#include <mqtt/client/mqtt_version_mapping.h>

namespace reactormq::mqtt::client
{
    ConnectingState::ConnectingState(const bool cleanSession, std::promise<Result<void>> promise)
        : m_cleanSession(cleanSession)
        , m_promise(std::move(promise))
    {
    }

    StateTransition ConnectingState::onEnter(Context& context)
    {
        if (!context.getSocket())
        {
            context.setSocket(socket::CreateSocket(context.getSettings()));
        }

        if (const auto sock = context.getSocket())
        {
            sock->connect();
        }

        return StateTransition::noTransition();
    }

    void ConnectingState::onExit(Context& /*context*/)
    {
        if (m_promise.has_value())
        {
            m_promise.value().set_value(Result<void>::failure("Connection interrupted"));
            m_promise.reset();
        }
    }

    StateTransition ConnectingState::handleCommand(Context& /*context*/, Command& /*command*/)
    {
        return StateTransition::noTransition();
    }

    StateTransition ConnectingState::onSocketConnected(Context& context)
    {
        const auto settings = context.getSettings();
        if (!settings)
        {
            if (m_promise.has_value())
            {
                m_promise.value().set_value(Result<void>::failure("No connection settings"));
                m_promise.reset();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
        }

        const auto sock = context.getSocket();
        if (!sock)
        {
            if (m_promise.has_value())
            {
                m_promise.value().set_value(Result<void>::failure("No socket available"));
                m_promise.reset();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
        }

        const auto protocolVersion = context.getProtocolVersion();
        std::vector<std::byte> buffer;
        serialize::ByteWriter writer(buffer);

        std::string username;
        std::string password;
        std::string authMethod;
        std::vector<std::uint8_t> initialAuthData;

        if (const auto credProvider = settings->getCredentialsProvider())
        {
            const auto creds = credProvider->getCredentials();
            username = creds.username;
            password = creds.password;

            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                authMethod = credProvider->getAuthMethod();
                initialAuthData = credProvider->getInitialAuthData();
            }
        }

        withMqttVersion(
            protocolVersion,
            [&context, &settings, &writer, &username, &password, &cleanSession = m_cleanSession, &authMethod, &initialAuthData]<
                typename VersionTag>(VersionTag)
            {
                constexpr auto kV = VersionTag::value;
                packets::encodeConnectToWriter<kV>(
                    writer,
                    context.getEffectiveClientId(),
                    settings->getKeepAliveIntervalSeconds(),
                    username,
                    password,
                    cleanSession,
                    authMethod,
                    initialAuthData);
            });

        sock->send(buffer.data(), static_cast<std::uint32_t>(buffer.size()));

        m_handshakeDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(settings->getMqttConnectionTimeoutSeconds());

        return StateTransition::noTransition();
    }

    StateTransition ConnectingState::onSocketDisconnected(Context& /*context*/)
    {
        if (m_promise.has_value())
        {
            m_promise.value().set_value(Result<void>::failure("Connection failed"));
            m_promise.reset();
        }

        return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
    }

    StateTransition ConnectingState::onDataReceived(Context& context, const uint8_t* data, const uint32_t size)
    {
        const auto packet = context.parsePacket(data, size);
        if (packet == nullptr)
        {
            if (m_promise.has_value())
            {
                m_promise.value().set_value(Result<void>::failure("Failed to parse CONNACK packet"));
                m_promise.reset();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
        }

        if (!packet->isValid())
        {
            if (m_promise.has_value())
            {
                m_promise.value().set_value(Result<void>::failure("Invalid CONNACK packet"));
                m_promise.reset();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
        }

        switch (const auto packetType = packet->getPacketType())
        {
        case packets::PacketType::Auth:
            return processing::authentication::handle(context, *packet, m_promise);

        case packets::PacketType::ConnAck:
            return handleConnAck(context, *packet);

        default:
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Warn,
                    "Unexpected packet type %d in Connecting state (expected AUTH or CONNACK)",
                    packetType);

                if (const auto settings = context.getSettings(); settings && settings->isStrictMode())
                {
                    if (m_promise.has_value())
                    {
                        m_promise.value().set_value(Result<void>::failure("Unexpected packet (strict mode)"));
                        m_promise.reset();
                    }
                    return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
                }

                return StateTransition::noTransition();
            }
        }
    }

    void ConnectingState::assignClientId(Context& context, const packets::ConnAck<packets::ProtocolVersion::V5>& connAck)
    {
        const auto& properties = connAck.getProperties();
        for (const auto& prop : properties.getProperties())
        {
            if (prop.getIdentifier() != packets::properties::PropertyIdentifier::AssignedClientIdentifier)
            {
                continue;
            }

            std::string assignedClientId;
            if (prop.tryGetValue(assignedClientId))
            {
                context.setAssignedClientId(std::move(assignedClientId));
            }
        }
    }

    StateTransition ConnectingState::handleConnAck(Context& context, const packets::IControlPacket& packet)
    {
        bool success = false;

        if (const auto protocolVersion = context.getProtocolVersion(); protocolVersion == packets::ProtocolVersion::V5)
        {
            auto const* connAck = static_cast<const packets::ConnAck<packets::ProtocolVersion::V5>*>(&packet);
            success = nullptr == connAck ? false : connAck->getReasonCode() == ReasonCode::Success;

            if (success)
            {
                assignClientId(context, *connAck);
            }
        }
        else
        {
            auto const* connAck = static_cast<const packets::ConnAck<packets::ProtocolVersion::V311>*>(&packet);
            if (nullptr != connAck)
            {
                success = connAck->getReasonCode() == packets::ConnectReturnCode::Accepted;
            }
        }

        if (m_promise.has_value())
        {
            if (success)
            {
                m_promise.value().set_value(Result<void>::success());
            }
            else
            {
                m_promise.value().set_value(Result<void>::failure("Connection refused by broker"));
            }
            m_promise.reset();
        }

        if (success)
        {
            return StateTransition::transitionTo(std::make_unique<ReadyState>());
        }
        return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
    }

    StateTransition ConnectingState::onTick(Context& /*context*/)
    {
        if (m_handshakeDeadline.has_value() && std::chrono::steady_clock::now() > m_handshakeDeadline.value())
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "ConnectingState::onTick() handshake timeout");
            if (m_promise.has_value())
            {
                m_promise.value().set_value(Result<void>::failure("Handshake timeout"));
                m_promise.reset();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
        }
        return StateTransition::noTransition();
    }

    const char* ConnectingState::getStateName() const
    {
        return "Connecting";
    }
} // namespace reactormq::mqtt::client
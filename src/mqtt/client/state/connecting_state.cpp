#include "connecting_state.h"
#include "disconnected_state.h"
#include "ready_state.h"
#include "socket/socket_base.h"
#include "mqtt/packets/connect.h"
#include "mqtt/packets/conn_ack.h"
#include "mqtt/packets/auth.h"
#include "mqtt/packets/fixed_header.h"
#include "serialize/bytes.h"

#include <cstring>

namespace reactormq::mqtt::client
{
    ConnectingState::ConnectingState(const bool cleanSession, std::promise<Result<void>> promise)
        : m_cleanSession(cleanSession), m_promise(std::move(promise))
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

    void ConnectingState::onExit(Context& context)
    {
        if (m_promise.has_value())
        {
            m_promise.value().set_value(Result<void>::failure("Connection interrupted"));
            m_promise.reset();
        }
    }

    StateTransition ConnectingState::handleCommand(Context& context, Command& command)
    {
        return StateTransition::noTransition();
    }

    StateTransition ConnectingState::onSocketConnected(Context& context)
    {
        auto settings = context.getSettings();
        if (!settings)
        {
            if (m_promise.has_value())
            {
                m_promise.value().set_value(Result<void>::failure("No connection settings"));
                m_promise.reset();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
        }

        auto sock = context.getSocket();
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

        if (auto credProvider = settings->getCredentialsProvider())
        {
            auto creds = credProvider->getCredentials();
            username = creds.username;
            password = creds.password;

            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                authMethod = credProvider->getAuthMethod();
                initialAuthData = credProvider->getInitialAuthData();
            }
        }

        if (protocolVersion == packets::ProtocolVersion::V5)
        {
            packets::properties::Properties connectProperties;

            if (!authMethod.empty())
            {
                std::vector<packets::properties::Property> propList;
                propList.emplace_back(
                    packets::properties::Property::create<
                        packets::properties::PropertyIdentifier::AuthenticationMethod,
                        std::string>(authMethod));

                if (!initialAuthData.empty())
                {
                    propList.emplace_back(
                        packets::properties::Property::create<
                            packets::properties::PropertyIdentifier::AuthenticationData,
                            std::vector<std::uint8_t>>(initialAuthData));
                }

                connectProperties = packets::properties::Properties(propList);
            }

            packets::Connect<packets::ProtocolVersion::V5> connectPacket(
                settings->getClientId(),
                settings->getKeepAliveIntervalSeconds(),
                username,
                password,
                m_cleanSession,
                false,
                {},
                {},
                QualityOfService::AtMostOnce,
                {},
                connectProperties
                );
            connectPacket.encode(writer);
        }
        else
        {
            packets::Connect<packets::ProtocolVersion::V311> connectPacket(
                settings->getClientId(),
                settings->getKeepAliveIntervalSeconds(),
                username,
                password,
                m_cleanSession,
                false,
                {},
                {},
                QualityOfService::AtMostOnce
                );
            connectPacket.encode(writer);
        }

        sock->send(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));

        return StateTransition::noTransition();
    }

    StateTransition ConnectingState::onSocketDisconnected(Context& context)
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
        auto packet = context.parsePacket(data, size);
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

        const auto packetType = packet->getPacketType();
        const auto protocolVersion = context.getProtocolVersion();

        switch (packetType)
        {
        case packets::PacketType::Auth: {
            if (protocolVersion != packets::ProtocolVersion::V5)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "AUTH packet received but protocol is not MQTT 5");
                if (m_promise.has_value())
                {
                    m_promise.value().set_value(Result<void>::failure("AUTH not supported in MQTT 3.1.1"));
                    m_promise.reset();
                }
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }

            auto* authPacket = static_cast<packets::Auth*>(packet.get());
            if (!authPacket)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "Failed to cast to AUTH packet");
                if (m_promise.has_value())
                {
                    m_promise.value().set_value(Result<void>::failure("Invalid AUTH packet"));
                    m_promise.reset();
                }
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }

            const auto reasonCode = authPacket->getReasonCode();
            if (reasonCode != ReasonCode::ContinueAuthentication)
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Error,
                    "AUTH packet with unexpected reason code: %d",
                    static_cast<int>(reasonCode));
                if (m_promise.has_value())
                {
                    m_promise.value().set_value(Result<void>::failure("Authentication failed"));
                    m_promise.reset();
                }
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }

            std::vector<std::uint8_t> serverAuthData;
            const auto& authProps = authPacket->getProperties();
            for (const auto& prop : authProps.getProperties())
            {
                if (prop.getIdentifier() == packets::properties::PropertyIdentifier::AuthenticationData)
                {
                    prop.tryGetValue(serverAuthData);
                    break;
                }
            }

            auto settings = context.getSettings();
            if (!settings || !settings->getCredentialsProvider())
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "No credentials provider available for AUTH challenge");
                if (m_promise.has_value())
                {
                    m_promise.value().set_value(Result<void>::failure("No credentials provider"));
                    m_promise.reset();
                }
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }

            auto credProvider = settings->getCredentialsProvider();
            std::vector<std::uint8_t> clientAuthData = credProvider->onAuthChallenge(serverAuthData);

            std::vector<packets::properties::Property> responsePropList;
            if (!clientAuthData.empty())
            {
                responsePropList.emplace_back(
                    packets::properties::Property::create<
                        packets::properties::PropertyIdentifier::AuthenticationData,
                        std::vector<std::uint8_t>>(clientAuthData));
            }
            packets::properties::Properties responseProps(responsePropList);

            packets::Auth authResponse(ReasonCode::ContinueAuthentication, responseProps);

            std::vector<std::byte> responseBuffer;
            serialize::ByteWriter responseWriter(responseBuffer);
            authResponse.encode(responseWriter);

            auto sock = context.getSocket();
            if (sock)
            {
                sock->send(
                    reinterpret_cast<const std::uint8_t*>(responseBuffer.data()),
                    static_cast<std::uint32_t>(responseBuffer.size()));
            }

            return StateTransition::noTransition();
        }

        case packets::PacketType::ConnAck: {
            bool success = false;
            if (protocolVersion == packets::ProtocolVersion::V5)
            {
                auto* connAck = static_cast<packets::ConnAck<packets::ProtocolVersion::V5>*>(packet.get());
                if (connAck)
                {
                    success = (connAck->getReasonCode() == ReasonCode::Success);
                }
            }
            else
            {
                auto* connAck = static_cast<packets::ConnAck<packets::ProtocolVersion::V311>*>(packet.get());
                if (connAck)
                {
                    success = (connAck->getReasonCode() == packets::ConnectReturnCode::Accepted);
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
                context.invokeCallback(
                    [&ctx = context]()
                    {
                        ctx.getOnConnect().broadcast(true);
                    });
                return StateTransition::transitionTo(std::make_unique<ReadyState>());
            }
            else
            {
                return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
            }
        }

        default: {
            REACTORMQ_LOG(
                logging::LogLevel::Warn,
                "Unexpected packet type %d in Connecting state (expected AUTH or CONNACK)",
                static_cast<int>(packetType));

            auto settings = context.getSettings();
            if (settings && settings->isStrictMode())
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

    StateTransition ConnectingState::onTick(Context& context)
    {
        return StateTransition::noTransition();
    }
} // namespace reactormq::mqtt::client
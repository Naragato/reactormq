//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "authentication_handler.h"

#include "mqtt/client/context.h"
#include "mqtt/client/state/disconnected_state.h"
#include "mqtt/packets/auth.h"
#include "mqtt/packets/properties/property.h"
#include "serialize/bytes.h"
#include "socket/socket.h"
#include "util/logging/logging.h"

namespace reactormq::mqtt::client::processing::authentication
{
    StateTransition handle(
        const Context& context, const packets::IControlPacket& packet, std::optional<std::promise<Result<void>>>& promise)
    {
        if (const auto protocolVersion = context.getProtocolVersion(); protocolVersion != packets::ProtocolVersion::V5)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "AUTH packet received but protocol is not MQTT 5");
            if (promise.has_value())
            {
                promise.value().set_value(Result<void>::failure("AUTH not supported in MQTT 3.1.1"));
                promise.reset();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
        }

        auto const* authPacket = static_cast<const packets::Auth*>(&packet);
        if (nullptr == authPacket)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "Failed to cast to AUTH packet");
            if (promise.has_value())
            {
                promise.value().set_value(Result<void>::failure("Invalid AUTH packet"));
                promise.reset();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
        }

        if (const auto reasonCode = authPacket->getReasonCode(); reasonCode != ReasonCode::ContinueAuthentication)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "AUTH packet with unexpected reason code: %d", reasonCode);
            if (promise.has_value())
            {
                promise.value().set_value(Result<void>::failure("Authentication failed"));
                promise.reset();
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

        const auto settings = context.getSettings();
        if (!settings || !settings->getCredentialsProvider())
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "No credentials provider available for AUTH challenge");
            if (promise.has_value())
            {
                promise.value().set_value(Result<void>::failure("No credentials provider"));
                promise.reset();
            }
            return StateTransition::transitionTo(std::make_unique<DisconnectedState>(false));
        }

        const auto credProvider = settings->getCredentialsProvider();
        const std::vector<std::uint8_t> clientAuthData = credProvider->onAuthChallenge(serverAuthData);

        std::vector<packets::properties::Property> responsePropList;
        if (!clientAuthData.empty())
        {
            responsePropList.emplace_back(
                packets::properties::Property::
                    create<packets::properties::PropertyIdentifier::AuthenticationData, std::vector<std::uint8_t>>(clientAuthData));
        }
        const packets::properties::Properties responseProps(responsePropList);

        const packets::Auth authResponse(ReasonCode::ContinueAuthentication, responseProps);

        std::vector<std::byte> responseBuffer;
        serialize::ByteWriter responseWriter(responseBuffer);
        authResponse.encode(responseWriter);

        if (const auto sock = context.getSocket())
        {
            sock->send(responseBuffer.data(), static_cast<std::uint32_t>(responseBuffer.size()));
        }

        return StateTransition::noTransition();
    }
} // namespace reactormq::mqtt::client::processing::authentication
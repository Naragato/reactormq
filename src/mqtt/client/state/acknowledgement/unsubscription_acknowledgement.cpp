//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/client/state/acknowledgement/unsubscription_acknowledgement.h"

#include "mqtt/client/command.h"
#include "mqtt/packets/unsub_ack.h"

namespace reactormq::mqtt::client::acknowledgement::unsubscription
{
    namespace
    {
        std::vector<UnsubscribeResult> resolveUnsubscriptionV5(
            const packets::IControlPacket& packet, const std::vector<std::string>& topics)
        {
            const auto* unsuback = static_cast<const packets::UnsubAck<packets::ProtocolVersion::V5>*>(&packet);
            const auto& reasonCodes = unsuback->getReasonCodes();
            const size_t count = std::min(reasonCodes.size(), topics.size());

            std::vector<UnsubscribeResult> results;
            results.reserve(count);

            for (size_t i = 0; i < count; ++i)
            {
                const bool wasSuccessful = (reasonCodes[i] == ReasonCode::Success);
                TopicFilter filter{ topics[i] };
                UnsubscribeResult result{ std::move(filter), wasSuccessful };
                results.emplace_back(std::move(result));
            }

            return results;
        }

        std::vector<UnsubscribeResult> resolveUnsubscriptionV311(const std::vector<std::string>& topics)
        {
            std::vector<UnsubscribeResult> results;
            results.reserve(topics.size());

            for (const auto& topic : topics)
            {
                TopicFilter filter{ topic };
                UnsubscribeResult result{ std::move(filter), false };
                results.emplace_back(std::move(result));
            }

            return results;
        }
    } // namespace

    void resolve(const packets::IControlPacket& packet, UnsubscribesCommand& unsubscription, const packets::ProtocolVersion protocolVersion)
    {
        std::vector<UnsubscribeResult> results;

        if (protocolVersion == packets::ProtocolVersion::V5)
        {
            results = resolveUnsubscriptionV5(packet, unsubscription.topics);
        }
        else
        {
            results = resolveUnsubscriptionV311(unsubscription.topics);
        }

        unsubscription.promise.set_value(Result<std::vector<UnsubscribeResult>>::success(std::move(results)));
    }
} // namespace reactormq::mqtt::client::acknowledgement::unsubscription
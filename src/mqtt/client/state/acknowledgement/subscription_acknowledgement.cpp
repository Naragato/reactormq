//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/client/state/acknowledgement/subscription_acknowledgement.h"

#include "mqtt/client/command.h"
#include "mqtt/packets/sub_ack.h"

namespace reactormq::mqtt::client::acknowledgement::subscription
{
    namespace
    {
        template<typename Codes, typename IsSuccessFn>
        void resolveSubscriptionPromises(
            std::optional<SubscribeCommand>& singleSubscription,
            std::optional<SubscribesCommand>& multiSubscription,
            const Codes& codes,
            IsSuccessFn&& isSuccess)
        {
            if (singleSubscription.has_value())
            {
                if (codes.empty())
                {
                    singleSubscription->promise.set_value(Result<SubscribeResult>::failure("Empty SUBACK"));
                }
                else
                {
                    SubscribeResult r{ singleSubscription->topicFilter, isSuccess(codes[0]) };
                    singleSubscription->promise.set_value(Result<SubscribeResult>::success(std::move(r)));
                }
                return;
            }

            std::vector<SubscribeResult> results;
            results.reserve(multiSubscription->topicFilters.size());

            for (size_t i = 0; i < codes.size() && i < multiSubscription->topicFilters.size(); ++i)
            {
                results.emplace_back(multiSubscription->topicFilters[i], isSuccess(codes[i]));
            }

            multiSubscription->promise.set_value(Result<std::vector<SubscribeResult>>::success(std::move(results)));
        }

        void resolveSubscriptionV5(
            const packets::IControlPacket& packet,
            std::optional<SubscribeCommand>& singleSubscription,
            std::optional<SubscribesCommand>& multiSubscription)
        {
            const auto* subAck = static_cast<const packets::SubAck<packets::ProtocolVersion::V5>*>(&packet);
            const auto& reasonCodes = subAck->getReasonCodes();

            resolveSubscriptionPromises(
                singleSubscription,
                multiSubscription,
                reasonCodes,
                [](const ReasonCode rc)
                {
                    using enum ReasonCode;
                    return rc == Success || rc == GrantedQualityOfService1 || rc == GrantedQualityOfService2;
                });
        }

        void resolveSubscriptionV311(
            const packets::IControlPacket& packet,
            std::optional<SubscribeCommand>& singleSubscription,
            std::optional<SubscribesCommand>& multiSubscription)
        {
            const auto* subAck = static_cast<const packets::SubAck<packets::ProtocolVersion::V311>*>(&packet);
            const auto& returnCodes = subAck->getReasonCodes();

            resolveSubscriptionPromises(
                singleSubscription,
                multiSubscription,
                returnCodes,
                [](const SubscribeReturnCode rc)
                {
                    return rc != SubscribeReturnCode::Failure;
                });
        }
    } // namespace

    void resolve(
        const packets::IControlPacket& packet,
        std::optional<SubscribeCommand>& singleSubscription,
        std::optional<SubscribesCommand>& multiSubscription,
        const packets::ProtocolVersion protocolVersion)
    {
        if (protocolVersion == packets::ProtocolVersion::V5)
        {
            resolveSubscriptionV5(packet, singleSubscription, multiSubscription);
        }
        else
        {
            resolveSubscriptionV311(packet, singleSubscription, multiSubscription);
        }
    }
} // namespace reactormq::mqtt::client::acknowledgement::subscription
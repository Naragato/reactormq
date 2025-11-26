//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/client/reactor.h"
#include "reactormq/mqtt/client.h"

#include <memory>

namespace reactormq::mqtt::client
{
    /**
     * @brief Implementation of the MQTT client interface.
     *
     * Delegates all operations to the internal Reactor.
     * Provides the public API that enqueues commands and returns futures.
     */
    class ClientImpl final
        : public IClient
        , public std::enable_shared_from_this<ClientImpl>
    {
    public:
        /**
         * @brief Construct a client implementation bound to connection settings.
         * @param settings Shared connection settings used by the reactor.
         */
        explicit ClientImpl(const ConnectionSettingsPtr& settings);

        ~ClientImpl() override;

        ConnectFuture connectAsync(bool cleanSession) override;

        DisconnectFuture disconnectAsync() override;

        PublishFuture publishAsync(Message&& message) override;

        SubscribesFuture subscribeAsync(const std::vector<TopicFilter>& topicFilters) override;

        SubscribeFuture subscribeAsync(TopicFilter&& topicFilter) override;

        SubscribeFuture subscribeAsync(const std::string& topicFilter) override;

        UnsubscribesFuture unsubscribeAsync(const std::vector<std::string>& topics) override;

        OnConnect& onConnect() override;

        OnDisconnect& onDisconnect() override;

        OnPublish& onPublish() override;

        OnSubscribe& onSubscribe() override;

        OnUnsubscribe& onUnsubscribe() override;

        OnMessage& onMessage() override;

        [[nodiscard]] bool isConnected() const override;

        void closeSocket(std::int32_t code, const std::string& reason) override;

        /**
         * @brief Tick the reactor (for polling mode).
         * Call this periodically from the same thread.
         */
        void tick() override;

    private:
        std::shared_ptr<Reactor> m_reactor;
    };
} // namespace reactormq::mqtt::client
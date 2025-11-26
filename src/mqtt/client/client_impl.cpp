//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "client_impl.h"

#include "command.h"

#include <reactormq/mqtt/connection_settings.h>

namespace reactormq::mqtt::client
{
    ClientImpl::ClientImpl(const ConnectionSettingsPtr& settings)
        : m_reactor(std::make_shared<Reactor>(settings))
    {
    }

    ClientImpl::~ClientImpl() = default;

    ConnectFuture ClientImpl::connectAsync(const bool cleanSession)
    {
        std::promise<Result<void>> promise;
        auto future = promise.get_future();

        ConnectCommand cmd{ cleanSession, std::move(promise) };
        m_reactor->enqueueCommand(std::move(cmd));

        return future;
    }

    DisconnectFuture ClientImpl::disconnectAsync()
    {
        std::promise<Result<void>> promise;
        auto future = promise.get_future();

        DisconnectCommand cmd{ std::move(promise) };
        m_reactor->enqueueCommand(std::move(cmd));

        return future;
    }

    PublishFuture ClientImpl::publishAsync(Message&& message)
    {
        std::promise<Result<void>> promise;
        auto future = promise.get_future();

        PublishCommand cmd{ std::move(message), std::move(promise) };
        m_reactor->enqueueCommand(std::move(cmd));

        return future;
    }

    SubscribesFuture ClientImpl::subscribeAsync(const std::vector<TopicFilter>& topicFilters)
    {
        std::promise<Result<std::vector<SubscribeResult>>> promise;
        auto future = promise.get_future();

        SubscribesCommand cmd{ topicFilters, std::move(promise) };
        m_reactor->enqueueCommand(std::move(cmd));

        return future;
    }

    SubscribeFuture ClientImpl::subscribeAsync(TopicFilter&& topicFilter)
    {
        std::promise<Result<SubscribeResult>> promise;
        auto future = promise.get_future();

        SubscribeCommand cmd{ std::move(topicFilter), std::move(promise) };
        m_reactor->enqueueCommand(std::move(cmd));

        return future;
    }

    SubscribeFuture ClientImpl::subscribeAsync(const std::string& topicFilter)
    {
        return subscribeAsync(TopicFilter{ topicFilter, QualityOfService::AtLeastOnce });
    }

    UnsubscribesFuture ClientImpl::unsubscribeAsync(const std::vector<std::string>& topics)
    {
        std::promise<Result<std::vector<UnsubscribeResult>>> promise;
        auto future = promise.get_future();

        UnsubscribesCommand cmd{ topics, std::move(promise) };
        m_reactor->enqueueCommand(std::move(cmd));

        return future;
    }

    OnConnect& ClientImpl::onConnect()
    {
        return m_reactor->getContext().getOnConnect();
    }

    OnDisconnect& ClientImpl::onDisconnect()
    {
        return m_reactor->getContext().getOnDisconnect();
    }

    OnPublish& ClientImpl::onPublish()
    {
        return m_reactor->getContext().getOnPublish();
    }

    OnSubscribe& ClientImpl::onSubscribe()
    {
        return m_reactor->getContext().getOnSubscribe();
    }

    OnUnsubscribe& ClientImpl::onUnsubscribe()
    {
        return m_reactor->getContext().getOnUnsubscribe();
    }

    OnMessage& ClientImpl::onMessage()
    {
        return m_reactor->getContext().getOnMessage();
    }

    bool ClientImpl::isConnected() const
    {
        return m_reactor->isConnected();
    }

    void ClientImpl::closeSocket(const std::int32_t code, const std::string& reason)
    {
        CloseSocketCommand cmd{ code, reason };
        m_reactor->enqueueCommand(std::move(cmd));
    }

    void ClientImpl::tick()
    {
        m_reactor->tick();
    }
} // namespace reactormq::mqtt::client
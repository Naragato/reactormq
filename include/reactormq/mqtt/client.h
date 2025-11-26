//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/export.h"
#include "reactormq/mqtt/connectable_async.h"
#include "reactormq/mqtt/delegates.h"
#include "reactormq/mqtt/disconnectable_async.h"
#include "reactormq/mqtt/publishable_async.h"
#include "reactormq/mqtt/subscribable_async.h"
#include "reactormq/mqtt/unsubscribable_async.h"

#include <string>

namespace reactormq::mqtt
{
    /**
     * @brief MQTT client interface
     * Aggregates async capabilities and exposes event callbacks.
     */
    class REACTORMQ_API IClient
        : public IConnectableAsync
        , public IDisconnectableAsync
        , public IPublishableAsync
        , public ISubscribableAsync
        , public IUnsubscribableAsync
    {
    public:
        ~IClient() override = default;

        /**
         * @brief Access the connect event callback.
         * @return Reference to the OnConnect delegate to assign a handler.
         */
        virtual OnConnect& onConnect() = 0;

        /**
         * @brief Access the disconnect event callback.
         * @return Reference to the OnDisconnect delegate to assign a handler.
         */
        virtual OnDisconnect& onDisconnect() = 0;

        /**
         * @brief Access the publish completion event callback.
         * @return Reference to the OnPublish delegate to assign a handler.
         */
        virtual OnPublish& onPublish() = 0;

        /**
         * @brief Access the subscribe completion event callback.
         * @return Reference to the OnSubscribe delegate to assign a handler.
         */
        virtual OnSubscribe& onSubscribe() = 0;

        /**
         * @brief Access the unsubscribe completion event callback.
         * @return Reference to the OnUnsubscribe delegate to assign a handler.
         */
        virtual OnUnsubscribe& onUnsubscribe() = 0;

        /**
         * @brief Access the incoming message event callback.
         * @return Reference to the OnMessage delegate to assign a handler.
         */
        virtual OnMessage& onMessage() = 0;

        /**
         * @brief Check whether the client is currently connected to the broker.
         * @return True if connected, false otherwise.
         */
        [[nodiscard]] virtual bool isConnected() const = 0;

        /**
         * @brief Optional convenience to close the underlying socket/transport.
         * @param code Transport-specific close code (e.g., WebSocket close code or 0 for TCP).
         * @param reason Human-readable reason for closing, for diagnostics.
         */
        virtual void closeSocket(int32_t code, const std::string& reason) = 0;

        /**
         * @brief Tick the client reactor once (polling mode).
         * Call this periodically from your main loop if not using a background thread.
         */
        virtual void tick() = 0;
    };
} // namespace reactormq::mqtt
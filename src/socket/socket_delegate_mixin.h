//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "socket.h"

namespace reactormq::socket
{
    /**
     * @brief CRTP mixin that provides delegate storage and getters for socket implementations.
     * Reduces duplication by centralizing event delegate members and accessors.
     * @tparam Derived The concrete socket class (for CRTP).
     */
    template<class Derived>
    class SocketDelegateMixin : public Socket
    {
    public:
        using Socket::Socket;

        /**
         * @brief Get the connection events delegate.
         * @return Reference to the OnConnectCallback delegate.
         */
        OnConnectCallback& getOnConnectCallback() override
        {
            return m_onConnectCallback;
        }

        /**
         * @brief Get the disconnection events delegate.
         * @return Reference to the OnDisconnectCallback delegate.
         */
        OnDisconnectCallback& getOnDisconnectCallback() override
        {
            return m_onDisconnectCallback;
        }

        /**
         * @brief Get the data received events delegate.
         * @return Reference to the OnDataReceivedCallback delegate.
         */
        OnDataReceivedCallback& getOnDataReceivedCallback() override
        {
            return m_onDataReceivedCallback;
        }

    private:
        OnConnectCallback m_onConnectCallback;
        OnDisconnectCallback m_onDisconnectCallback;
        OnDataReceivedCallback m_onDataReceivedCallback;
    };
} // namespace reactormq::socket
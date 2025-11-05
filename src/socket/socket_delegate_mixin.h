#pragma once

#include "socket_base.h"

namespace reactormq::socket
{
    /**
     * @brief CRTP mixin that provides delegate storage and getters for socket implementations.
     * 
     * This template eliminates code duplication across concrete socket classes by
     * providing a single implementation of delegate members and their getters.
     * 
     * Usage:
     *   class MySocket : public SocketDelegateMixin<MySocket>
     *   {
     *       // ... implement other SocketBase methods
     *   };
     * 
     * @tparam Derived The concrete socket class (for CRTP).
     */
    template<class Derived>
    class SocketDelegateMixin : public SocketBase
    {
    public:
        using SocketBase::SocketBase;

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

//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "socket/secure_socket.h"

#include "socket/platform/socket_error.h"

#if REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS
#include "socket/platform/platform_secure_socket.h"
#endif // REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS

#include <algorithm>

namespace reactormq::socket
{
    SecureSocket::SecureSocket(mqtt::ConnectionSettingsPtr settings)
        : Socket(std::move(settings))
    {
        if (settings)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Debug,
                "SecureSocket::SecureSocket created (host=%s, port=%u, clientId=%s)",
                settings->getHost().c_str(),
                settings->getPort(),
                settings->getClientId().c_str());
        }
        else
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "SecureSocket::SecureSocket created with null settings");
        }
    }

    SecureSocket::~SecureSocket()
    {
        REACTORMQ_LOG(
            logging::LogLevel::Debug,
            "SecureSocket::~SecureSocket (connected=%s)",
            (m_socketPtr && m_socketPtr->isConnected()) ? "true" : "false");

        m_socketPtr.reset();
    }

    void SecureSocket::connect()
    {
        auto self = shared_from_this();
        {
            mqtt::ConnectionSettingsPtr settings = getSettings();
            std::scoped_lock lock(m_resourceMutex);
            if (m_socketPtr && m_socketPtr->isConnected())
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Warn,
                    "SecureSocket::connect() called, but already connected (host=%s, port=%u, clientId=%s)",
                    settings ? settings->getHost().c_str() : "<null>",
                    settings ? static_cast<unsigned>(settings->getPort()) : 0U,
                    settings ? settings->getClientId().c_str() : "<null>");
                return;
            }

            if (!settings)
            {
                REACTORMQ_LOG(logging::LogLevel::Error, "SecureSocket::connect() called with null settings");
                m_connectCallbackInvoked.store(true, std::memory_order_release);
                invokeOnConnect(false);
                return;
            }

            const std::string host = settings->getHost();
            const std::uint16_t port = settings->getPort();

            REACTORMQ_LOG(
                logging::LogLevel::Info,
                "SecureSocket::connect() starting (host=%s, port=%u, clientId=%s)",
                host.c_str(),
                port,
                settings->getClientId().c_str());

#if REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS
            if (const mqtt::ConnectionProtocol protocol = settings->getProtocol();
                protocol == mqtt::ConnectionProtocol::Tls || protocol == mqtt::ConnectionProtocol::Wss)
            {
                REACTORMQ_LOG(logging::LogLevel::Debug, "SecureSocket::connect() using PlatformSecureSocket (protocol=%d)", protocol);
                m_socketPtr = std::make_unique<PlatformSecureSocket>(settings);
            }
            else
#endif
            {
                REACTORMQ_LOG(logging::LogLevel::Debug, "SecureSocket::connect() using PlatformSocket (non-TLS)");
                m_socketPtr = std::make_unique<PlatformSocket>();
            }

            if (const int result = m_socketPtr->connect(host, port); result != 0)
            {
                const int32_t error = PlatformSocket::getLastErrorCode();
                REACTORMQ_LOG(
                    logging::LogLevel::Error,
                    "SecureSocket::connect() failed (host=%s, port=%u, result=%d, error=%d: %s)",
                    host.c_str(),
                    port,
                    result,
                    error,
                    PlatformSocket::getNetworkErrorDescription(error));
                m_connectCallbackInvoked.store(true, std::memory_order_release);
                invokeOnConnect(false);
                return;
            }

            if (m_socketPtr->isConnected())
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Info,
                    "SecureSocket::connect() succeeded (host=%s, port=%u, clientId=%s)",
                    host.c_str(),
                    port,
                    settings->getClientId().c_str());
                m_connectCallbackInvoked.store(true, std::memory_order_release);
                invokeOnConnect(true);
            }
            else
            {
                REACTORMQ_LOG(logging::LogLevel::Warn, "SecureSocket::connect() returned success but socket not reported as connected");
            }
        }
    }

    void SecureSocket::disconnect()
    {
        bool shouldInvokeCallback = false;

        {
            std::scoped_lock lock(m_resourceMutex);
            if (nullptr == m_socketPtr)
            {
                REACTORMQ_LOG(logging::LogLevel::Debug, "SecureSocket::disconnect() called, but socket pointer is null");
            }
            else
            {
                const mqtt::ConnectionSettingsPtr settings = getSettings();
                REACTORMQ_LOG(
                    logging::LogLevel::Info,
                    "SecureSocket::disconnect() closing connection (host=%s, clientId=%s)",
                    settings ? settings->getHost().c_str() : "<null>",
                    settings ? settings->getClientId().c_str() : "<null>");

                shouldInvokeCallback = true;
                m_socketPtr.reset();
                m_connectCallbackInvoked.store(false, std::memory_order_release);
            }
        }

        if (shouldInvokeCallback)
        {
            REACTORMQ_LOG(logging::LogLevel::Debug, "SecureSocket::disconnect() invoking disconnect callback");
            invokeOnDisconnect();
        }
    }

    void SecureSocket::close(int32_t /*code*/, const std::string& reason)
    {
        const mqtt::ConnectionSettingsPtr settings = getSettings();
        REACTORMQ_LOG(
            logging::LogLevel::Info,
            "SecureSocket::close() called (reason=%s, host=%s, clientId=%s)",
            reason.c_str(),
            settings ? settings->getHost().c_str() : "<null>",
            settings ? settings->getClientId().c_str() : "<null>");
        disconnect();
    }

    bool SecureSocket::isConnected() const
    {
        const bool connected = m_socketPtr && m_socketPtr->isConnected();
        REACTORMQ_LOG(logging::LogLevel::Trace, "SecureSocket::isConnected() -> %s", connected ? "true" : "false");
        return connected;
    }

    void SecureSocket::send(const uint8_t* data, const uint32_t size)
    {
        const mqtt::ConnectionSettingsPtr settings = getSettings();
        if (data == nullptr || size == 0)
        {
            REACTORMQ_LOG(logging::LogLevel::Debug, "SecureSocket::send() called with empty or null data");
            return;
        }
        if (!settings)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "SecureSocket::send() called while settings are null");
            return;
        }
        if (nullptr == m_socketPtr)
        {
            REACTORMQ_LOG(
                logging::LogLevel::Error,
                "SecureSocket::send() called with null socket (host=%s, clientId=%s)",
                settings->getHost().c_str(),
                settings->getClientId().c_str());
            return;
        }

        size_t totalBytesSent = 0;
        while (totalBytesSent < static_cast<size_t>(size))
        {
            size_t bytesSent = 0;
            if (const auto remaining = size - static_cast<uint32_t>(totalBytesSent);
                !m_socketPtr->trySend(data + totalBytesSent, remaining, bytesSent))
            {
                const int32_t errorCode = PlatformSocket::getLastErrorCode();
                REACTORMQ_LOG(
                    logging::LogLevel::Error,
                    "SecureSocket::send(): trySend failed (error=%d: %s)",
                    errorCode,
                    PlatformSocket::getNetworkErrorDescription(errorCode));
                disconnect();
                return;
            }
            if (bytesSent == 0)
            {
                if (const SocketError err = PlatformSocket::getLastError(); err == SocketError::WouldBlock)
                {
                    REACTORMQ_LOG(logging::LogLevel::Trace, "SecureSocket::send(): WouldBlock, deferring");
                    return;
                }
                const int32_t errorCode = PlatformSocket::getLastErrorCode();
                REACTORMQ_LOG(
                    logging::LogLevel::Error,
                    "SecureSocket::send(): zero bytes sent (error=%d: %s)",
                    errorCode,
                    PlatformSocket::getNetworkErrorDescription(errorCode));
                disconnect();
                return;
            }
            totalBytesSent += bytesSent;
        }
    }

    void SecureSocket::tick()
    {
        bool shouldDisconnect = false;
        bool shouldInvokeConnect = false;
        const mqtt::ConnectionSettingsPtr settings = getSettings();

        {
            std::scoped_lock lock(m_resourceMutex);
            if (nullptr == m_socketPtr)
            {
                REACTORMQ_LOG(logging::LogLevel::Trace, "SecureSocket::tick() called with null socket");
                return;
            }

            if (!m_connectCallbackInvoked.load(std::memory_order_acquire) && m_socketPtr->isConnected())
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Info,
                    "SecureSocket::tick() detected connection established (host=%s, clientId=%s)",
                    settings ? settings->getHost().c_str() : "<null>",
                    settings ? settings->getClientId().c_str() : "<null>");
                m_connectCallbackInvoked.store(true, std::memory_order_release);
                shouldInvokeConnect = true;
            }

            if (m_socketPtr && m_socketPtr->isConnected())
            {
                shouldDisconnect = !readAvailableData();
            }
        }

        if (shouldInvokeConnect)
        {
            REACTORMQ_LOG(logging::LogLevel::Debug, "SecureSocket::tick() invoking connect callback");
            invokeOnConnect(true);
        }

        if (shouldDisconnect)
        {
            REACTORMQ_LOG(logging::LogLevel::Info, "SecureSocket::tick() readAvailableData() requested disconnect");
            disconnect();
        }
    }

    bool SecureSocket::readAvailableData()
    {
        if (nullptr == m_socketPtr)
        {
            REACTORMQ_LOG(logging::LogLevel::Debug, "SecureSocket::readAvailableData() called with null socket");
            return false;
        }

        const int pendingData = m_socketPtr->getPendingData();

        int chunkSize = pendingData > 0 ? std::min(pendingData, kMaxChunkSize) : kMaxChunkSize;

        std::vector<uint8_t> tempBuffer(chunkSize);
        size_t bytesRead = 0;

        if (!m_socketPtr->tryReceive(tempBuffer.data(), chunkSize, bytesRead))
        {
            if (const SocketError err = PlatformSocket::getLastError(); err == SocketError::WouldBlock)
            {
                return true;
            }

            if (const int32_t errorCode = PlatformSocket::getLastErrorCode(); errorCode != 0)
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Error,
                    "SecureSocket::readAvailableData() tryReceive failed (chunkSize=%u, error=%d: %s)",
                    chunkSize,
                    errorCode,
                    PlatformSocket::getNetworkErrorDescription(errorCode));
            }
            else
            {
                REACTORMQ_LOG(logging::LogLevel::Debug, "SecureSocket::readAvailableData() detected socket closure (EOF)");
            }

            return false;
        }

        if (bytesRead == 0)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "SecureSocket::readAvailableData() bytesRead=0");
            return true;
        }

        return processPacketData(tempBuffer.data(), bytesRead);
    }
} // namespace reactormq::socket
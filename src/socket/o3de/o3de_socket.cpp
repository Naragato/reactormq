#ifdef REACTORMQ_WITH_O3DE_SOCKET

#include "o3de_socket.h"

#include <AzNetworking/TcpTransport/TcpSocket.h>
#include <AzNetworking/TcpTransport/TlsSocket.h>
#include <AzNetworking/Utilities/IpAddress.h>
#include <AzCore/std/string/string.h>

namespace reactormq::socket
{
    O3deSocket::O3deSocket(const mqtt::ConnectionSettingsPtr& settings)
        : SocketDelegateMixin<O3deSocket>(settings)
        , m_socket(nullptr)
        , m_currentState(O3deSocketState::Disconnected)
        , m_useTls(settings->getProtocol() == mqtt::ConnectionProtocol::Mqtts)
    {
        if (m_useTls)
        {
            m_socket = std::make_unique<AzNetworking::TlsSocket>(AzNetworking::TrustZone::ExternalClientConnection);
        }
        else
        {
            m_socket = std::make_unique<AzNetworking::TcpSocket>();
        }
    }

    O3deSocket::~O3deSocket()
    {
        disconnectInternal();
    }

    void O3deSocket::connect()
    {
        O3deSocketState expected = O3deSocketState::Disconnected;
        if (!m_currentState.compare_exchange_strong(
            expected,
            O3deSocketState::Connecting,
            std::memory_order_acq_rel,
            std::memory_order_acquire))
        {
            return;
        }

        {
            std::scoped_lock<std::mutex> lock(m_socketMutex);
            
            if (!m_socket)
            {
                m_currentState.store(O3deSocketState::Disconnected, std::memory_order_release);
                invokeOnConnect(false);
                return;
            }

            auto ipAddress = createIpAddress(m_settings->getHost(), m_settings->getPort());
            if (!ipAddress)
            {
                m_currentState.store(O3deSocketState::Disconnected, std::memory_order_release);
                invokeOnConnect(false);
                return;
            }

            if (m_socket->Connect(*ipAddress, 0))
            {
                m_currentState.store(O3deSocketState::Connected, std::memory_order_release);
            }
            else
            {
                m_currentState.store(O3deSocketState::Disconnected, std::memory_order_release);
            }
        }

        if (m_currentState.load(std::memory_order_acquire) == O3deSocketState::Connected)
        {
            invokeOnConnect(true);
        }
        else
        {
            invokeOnConnect(false);
            expected = O3deSocketState::Connecting;
            if (m_currentState.compare_exchange_strong(expected, O3deSocketState::Disconnected))
            {
                disconnectInternal();
            }
        }
    }

    void O3deSocket::disconnect()
    {
        while (m_currentState.load(std::memory_order_acquire) == O3deSocketState::Connecting)
        {
            std::this_thread::yield();
        }

        O3deSocketState expected = m_currentState.load(std::memory_order_acquire);
        while (expected == O3deSocketState::Connected)
        {
            if (m_currentState.compare_exchange_weak(expected, O3deSocketState::Disconnected))
            {
                disconnectInternal();
                invokeOnDisconnect();
                break;
            }
        }
    }

    void O3deSocket::close(int32_t code, const std::string& reason)
    {
        disconnect();
    }

    void O3deSocket::send(const uint8_t* data, uint32_t size)
    {
        std::scoped_lock<std::mutex> lock(m_socketMutex);
        if (!isConnected() || !m_socket)
        {
            return;
        }

        uint32_t totalBytesSent = 0;
        while (totalBytesSent < size)
        {
            const int32_t bytesSent = m_socket->Send(data + totalBytesSent, size - totalBytesSent);
            
            if (bytesSent > 0)
            {
                totalBytesSent += static_cast<uint32_t>(bytesSent);
            }
            else if (bytesSent == 0)
            {
                std::this_thread::yield();
            }
            else
            {
                disconnect();
                return;
            }
        }
    }

    void O3deSocket::tick()
    {
        bool shouldDisconnect = false;

        {
            std::scoped_lock<std::mutex> lock(m_socketMutex);

            const O3deSocketState state = m_currentState.load(std::memory_order_acquire);
            if (state == O3deSocketState::Disconnected)
            {
                return;
            }

            if (isConnected())
            {
                shouldDisconnect = !receiveData();
            }
        }

        if (shouldDisconnect)
        {
            disconnect();
        }
    }

    bool O3deSocket::isConnected() const
    {
        std::scoped_lock<std::mutex> lock(m_socketMutex);
        return (m_currentState.load(std::memory_order_acquire) == O3deSocketState::Connected)
            && m_socket
            && m_socket->IsOpen();
    }

    void O3deSocket::disconnectInternal()
    {
        std::scoped_lock<std::mutex> lock(m_socketMutex);
        if (m_socket)
        {
            m_socket->Close();
        }
    }

    bool O3deSocket::receiveData()
    {
        if (!m_socket)
        {
            return false;
        }

        std::vector<uint8_t> tempBuffer(kMaxChunkSize);

        const int32_t bytesReceived = m_socket->Receive(tempBuffer.data(), kMaxChunkSize);

        if (bytesReceived > 0)
        {
            bool shouldDisconnect = false;
            {
                m_dataBuffer.insert(m_dataBuffer.end(), tempBuffer.data(), tempBuffer.data() + bytesReceived);

                if (m_dataBuffer.size() > m_settings->getMaxBufferSize())
                {
                    shouldDisconnect = true;
                }
            }

            if (shouldDisconnect)
            {
                return false;
            }

            if (!readPacketsFromBuffer())
            {
                return false;
            }
        }
        else if (bytesReceived < 0)
        {
            return false;
        }

        return true;
    }

    std::unique_ptr<AzNetworking::IpAddress> O3deSocket::createIpAddress(const std::string& host, uint16_t port)
    {

        AZ::AZStd::string azHost(host.c_str());
        

        auto ipAddress = std::make_unique<AzNetworking::IpAddress>(azHost.c_str(), port, AzNetworking::ProtocolType::Tcp);
        
        return ipAddress;
    }

} // namespace reactormq::socket

#endif // REACTORMQ_WITH_O3DE_SOCKET

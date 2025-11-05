#include "native_socket.h"

#include "native_tcp_socket.h"
#include "native_tls_socket.h"
#include "config/platform_config.h"
#include "reactormq/mqtt/connection_protocol.h"

namespace reactormq::socket
{
    NativeSocket::NativeSocket(const mqtt::ConnectionSettingsPtr& settings) : SocketBase(settings)
    {
        const mqtt::ConnectionProtocol protocol = settings->getProtocol();

        if (protocol == mqtt::ConnectionProtocol::Tls || protocol == mqtt::ConnectionProtocol::Wss)
        {
            using TlsSocket = std::conditional_t<config::kHasOpenSsl, NativeTlsSocket, NativeTcpSocket>;
            m_socket = std::make_unique<TlsSocket>(settings);
        }
        else
        {
            m_socket = std::make_unique<NativeTcpSocket>(settings);
        }
    }

    void NativeSocket::connect()
    {
        m_socket->connect();
    }

    void NativeSocket::disconnect()
    {
        m_socket->disconnect();
    }

    void NativeSocket::close(const int32_t code, const std::string& reason)
    {
        m_socket->close(code, reason);
    }

    bool NativeSocket::isConnected() const
    {
        return m_socket->isConnected();
    }

    void NativeSocket::send(const uint8_t* data, const uint32_t size)
    {
        m_socket->send(data, size);
    }

    void NativeSocket::tick()
    {
        m_socket->tick();
    }
} // namespace reactormq::socket
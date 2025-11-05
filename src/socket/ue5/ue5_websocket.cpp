#ifdef REACTORMQ_WITH_WEBSOCKET_UE5

#include "ue5_websocket.h"

#include "WebSocketsModule.h"
#include "IWebSocket.h"

namespace reactormq::socket
{
    Ue5WebSocket::Ue5WebSocket(const mqtt::ConnectionSettingsPtr& settings)
        : SocketDelegateMixin<Ue5WebSocket>(settings)
        , m_webSocket(nullptr)
        , m_currentState(Ue5WebSocketState::Disconnected)
        , m_useTls(settings->getProtocol() == mqtt::ConnectionProtocol::Wss)
    {
    }

    Ue5WebSocket::~Ue5WebSocket()
    {
        disconnect();
    }

    void Ue5WebSocket::connect()
    {
        Ue5WebSocketState expected = Ue5WebSocketState::Disconnected;
        if (!m_currentState.compare_exchange_strong(
            expected,
            Ue5WebSocketState::Connecting,
            std::memory_order_acq_rel,
            std::memory_order_acquire))
        {
            return;
        }

        const std::string url = buildWebSocketUrl();
        
        TMap<FString, FString> headers;
        TArray<FString> protocols;
        protocols.Add(TEXT("mqtt"));

        m_webSocket = FWebSocketsModule::Get().CreateWebSocket(
            FString(url.c_str()),
            protocols,
            headers);

        if (!m_webSocket)
        {
            m_currentState.store(Ue5WebSocketState::Disconnected, std::memory_order_release);
            invokeOnConnect(false);
            return;
        }

        m_webSocket->OnConnected().AddRaw(this, &Ue5WebSocket::onConnected);
        m_webSocket->OnConnectionError().AddLambda([this](const FString& error)
        {
            onConnectionError(std::string(TCHAR_TO_UTF8(*error)));
        });
        m_webSocket->OnClosed().AddLambda([this](int32 statusCode, const FString& reason, bool wasClean)
        {
            onClosed(statusCode, std::string(TCHAR_TO_UTF8(*reason)), wasClean);
        });
        m_webSocket->OnMessage().AddLambda([this](const FString& message)
        {
        });
        m_webSocket->OnBinaryMessage().AddLambda([this](const void* data, SIZE_T size, bool isLastFragment)
        {
            onBinaryMessage(data, static_cast<size_t>(size), isLastFragment);
        });

        m_webSocket->Connect();
    }

    void Ue5WebSocket::disconnect()
    {
        Ue5WebSocketState expected = m_currentState.load(std::memory_order_acquire);
        while (expected == Ue5WebSocketState::Connected || expected == Ue5WebSocketState::Connecting)
        {
            if (m_currentState.compare_exchange_weak(expected, Ue5WebSocketState::Disconnected))
            {
                if (m_webSocket && m_webSocket->IsConnected())
                {
                    m_webSocket->Close();
                }
                m_webSocket.reset();
                invokeOnDisconnect();
                break;
            }
        }
    }

    void Ue5WebSocket::close(int32_t code, const std::string& reason)
    {
        if (m_webSocket && m_webSocket->IsConnected())
        {
            m_webSocket->Close(code, FString(reason.c_str()));
        }
        disconnect();
    }

    void Ue5WebSocket::send(const uint8_t* data, uint32_t size)
    {
        std::scoped_lock<std::mutex> lock(m_socketMutex);
        if (!isConnected())
        {
            return;
        }

        m_webSocket->Send(data, size, true);
    }

    void Ue5WebSocket::tick()
    {
        if (m_currentState.load(std::memory_order_acquire) == Ue5WebSocketState::Connected)
        {
            std::scoped_lock<std::mutex> lock(m_socketMutex);
            if (m_dataBuffer.size() > m_dataBufferReadOffset)
            {
                if (!readPacketsFromBuffer())
                {
                    disconnect();
                }
            }
        }
    }

    bool Ue5WebSocket::isConnected() const
    {
        return m_currentState.load(std::memory_order_acquire) == Ue5WebSocketState::Connected
            && m_webSocket
            && m_webSocket->IsConnected();
    }

    void Ue5WebSocket::onConnected()
    {
        m_currentState.store(Ue5WebSocketState::Connected, std::memory_order_release);
        invokeOnConnect(true);
    }

    void Ue5WebSocket::onConnectionError(const std::string& error)
    {
        Ue5WebSocketState expected = Ue5WebSocketState::Connecting;
        if (m_currentState.compare_exchange_strong(expected, Ue5WebSocketState::Disconnected))
        {
            invokeOnConnect(false);
        }
        else
        {
            disconnect();
        }
    }

    void Ue5WebSocket::onClosed(int32_t statusCode, const std::string& reason, bool wasClean)
    {
        disconnect();
    }

    void Ue5WebSocket::onMessage(const void* data, size_t size, size_t bytesRemaining)
    {
    }

    void Ue5WebSocket::onBinaryMessage(const void* data, size_t size, bool isLastFragment)
    {
        if (!data || size == 0)
        {
            return;
        }

        bool shouldDisconnect = false;
        {
            std::scoped_lock<std::mutex> lock(m_socketMutex);
            
            const uint8_t* bytes = static_cast<const uint8_t*>(data);
            m_dataBuffer.insert(m_dataBuffer.end(), bytes, bytes + size);

            if (m_dataBuffer.size() > m_settings->getMaxBufferSize())
            {
                shouldDisconnect = true;
            }
        }

        if (shouldDisconnect)
        {
            disconnect();
            return;
        }

        if (!readPacketsFromBuffer())
        {
            disconnect();
        }
    }

    std::string Ue5WebSocket::buildWebSocketUrl() const
    {
        std::string url;
        
        if (m_useTls)
        {
            url = "wss://";
        }
        else
        {
            url = "ws://";
        }

        url += m_settings->getHost();

        url += ":";
        url += std::to_string(m_settings->getPort());

        const std::string& path = m_settings->getPath();
        if (!path.empty())
        {
            if (path[0] != '/')
            {
                url += "/";
            }
            url += path;
        }
        else
        {
            url += "/";
        }

        return url;
    }

} // namespace reactormq::socket

#endif // REACTORMQ_WITH_WEBSOCKET_UE5

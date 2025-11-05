#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformAffinity.h"
#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Containers/Queue.h"
#include <atomic>

namespace reactormq::socket
{
    /**
     * @brief WebSocket server state enumeration.
     */
    enum class WebSocketServerState : uint8_t
    {
        Disconnected,
        Connecting,
        Connected
    };

    /**
     * @brief Simple WebSocket echo server for testing purposes.
     * 
     * NOTE: This is a simplified implementation for basic connectivity testing.
     * For full WebSocket protocol testing, use an external WebSocket server such as:
     * - libwebsockets-test-server
     * - Node.js ws module server
     * - Python websockets server
     * 
     * This implementation provides basic TCP-level connectivity testing and can be
     * extended for WebSocket frame handling if needed. It listens on a specified port
     * and echoes back any received data.
     * 
     * LIMITATIONS:
     * - Does not implement full WebSocket protocol handshake
     * - Does not handle WebSocket frames
     * - Suitable only for basic connectivity testing
     * 
     * For proper WebSocket testing, the LibWebSocketRunnable from Mqttify uses
     * libwebsockets which is not available in ReactorMQ. Use external server instead.
     */
    class Ue5WebSocketServerRunnable final : public FRunnable
    {
    public:
        /**
         * @brief Construct a WebSocket test server.
         * @param port Port to listen on (default: 9001).
         * @param address Address to bind to (default: "127.0.0.1").
         */
        explicit Ue5WebSocketServerRunnable(const uint16_t port = 9001, const FString& address = TEXT("127.0.0.1"))
            : m_isStopping(false)
            , m_connectionState(WebSocketServerState::Disconnected)
            , m_port(port)
            , m_address(address)
            , m_listeningSocket(nullptr)
            , m_clientSocket(nullptr)
            , m_thread(nullptr)
        {
            initializeServer();

            m_thread = FRunnableThread::Create(
                this,
                TEXT("Ue5WebSocketServerRunnable"),
                0,
                TPri_Normal,
                FPlatformAffinity::GetPoolThreadMask());
        }

        /**
         * @brief Destructor. Stops the server and cleans up resources.
         */
        virtual ~Ue5WebSocketServerRunnable() override
        {
            if (m_thread)
            {
                m_isStopping.store(true, std::memory_order_release);
                m_thread->WaitForCompletion();
                m_thread->Kill(true);
                delete m_thread;
                m_thread = nullptr;
            }

            cleanupServer();
        }

        /**
         * @brief Send data to the connected client.
         * @param data Pointer to data buffer.
         * @param size Size of data in bytes.
         */
        void send(const uint8_t* data, const uint32_t size)
        {
            if (m_clientSocket)
            {
                TArray<uint8_t> payload(data, size);
                m_sendQueue.Enqueue(payload);
            }
        }

        /**
         * @brief Check if a client is connected.
         * @return True if connected, false otherwise.
         */
        bool isConnected() const
        {
            return m_connectionState.load(std::memory_order_acquire) == WebSocketServerState::Connected;
        }

        /**
         * @brief Main thread run function. Handles connections and data.
         * @return Exit code (0 for normal completion).
         */
        virtual uint32 Run() override
        {
            while (!m_isStopping.load(std::memory_order_acquire))
            {
                if (!m_clientSocket)
                {
                    acceptConnection();
                }
                else
                {
                    receiveData();
                    sendQueuedData();
                }

                FPlatformProcess::SleepNoStats(0.001f);
            }
            return 0;
        }

        /**
         * @brief Stop the server thread.
         */
        virtual void Stop() override
        {
            FRunnable::Stop();
            m_isStopping.store(true, std::memory_order_release);
            m_connectionState.store(WebSocketServerState::Disconnected, std::memory_order_release);
        }

    private:
        /**
         * @brief Initialize the listening socket.
         */
        void initializeServer()
        {
            ISocketSubsystem* socketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
            if (!socketSubsystem)
            {
                return;
            }

            const TSharedPtr<FInternetAddr> addr = socketSubsystem->CreateInternetAddr();
            addr->SetPort(m_port);

            m_listeningSocket = socketSubsystem->CreateSocket(NAME_Stream, TEXT("WebSocketTestServer"), false);
            if (m_listeningSocket)
            {
                m_listeningSocket->SetReuseAddr(true);
                m_listeningSocket->SetNonBlocking(true);

                if (m_listeningSocket->Bind(*addr))
                {
                    m_listeningSocket->Listen(5);
                    m_connectionState.store(WebSocketServerState::Connecting, std::memory_order_release);
                }
            }
        }

        /**
         * @brief Accept incoming client connections.
         */
        void acceptConnection()
        {
            if (!m_listeningSocket)
            {
                return;
            }

            bool hasPendingConnection = false;
            if (m_listeningSocket->HasPendingConnection(hasPendingConnection) && hasPendingConnection)
            {
                ISocketSubsystem* socketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
                TSharedRef<FInternetAddr> clientAddr = socketSubsystem->CreateInternetAddr();
                
                m_clientSocket = m_listeningSocket->Accept(*clientAddr, TEXT("WebSocketClient"));
                if (m_clientSocket)
                {
                    m_clientSocket->SetNonBlocking(true);
                    m_connectionState.store(WebSocketServerState::Connected, std::memory_order_release);
                }
            }
        }

        /**
         * @brief Receive data from the connected client.
         */
        void receiveData()
        {
            if (!m_clientSocket)
            {
                return;
            }

            uint32 pendingDataSize = 0;
            if (m_clientSocket->HasPendingData(pendingDataSize) && pendingDataSize > 0)
            {
                TArray<uint8_t> buffer;
                buffer.SetNumUninitialized(pendingDataSize);

                int32_t bytesRead = 0;
                if (m_clientSocket->Recv(buffer.GetData(), pendingDataSize, bytesRead) && bytesRead > 0)
                {
                    m_sendQueue.Enqueue(TArray<uint8_t>(buffer.GetData(), bytesRead));
                }
            }
        }

        /**
         * @brief Send queued data to the client.
         */
        void sendQueuedData()
        {
            if (!m_clientSocket)
            {
                return;
            }

            TArray<uint8_t> payload;
            while (m_sendQueue.Dequeue(payload) && payload.Num() > 0)
            {
                int32_t bytesSent = 0;
                m_clientSocket->Send(payload.GetData(), payload.Num(), bytesSent);
            }
        }

        /**
         * @brief Clean up server resources.
         */
        void cleanupServer()
        {
            if (m_clientSocket)
            {
                m_clientSocket->Close();
                ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(m_clientSocket);
                m_clientSocket = nullptr;
            }

            if (m_listeningSocket)
            {
                m_listeningSocket->Close();
                ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(m_listeningSocket);
                m_listeningSocket = nullptr;
            }
        }

        /// @brief Atomic flag to signal thread stopping.
        std::atomic<bool> m_isStopping;

        /// @brief Current connection state.
        std::atomic<WebSocketServerState> m_connectionState;

        /// @brief Port to listen on.
        uint16_t m_port;

        /// @brief Address to bind to.
        FString m_address;

        /// @brief Listening socket for accepting connections.
        FSocket* m_listeningSocket;

        /// @brief Connected client socket.
        FSocket* m_clientSocket;

        /// @brief Background thread.
        FRunnableThread* m_thread;

        /// @brief Queue of data to send to client.
        TQueue<TArray<uint8_t>, EQueueMode::Mpsc> m_sendQueue;
    };
} // namespace reactormq::socket

#endif // WITH_DEV_AUTOMATION_TESTS

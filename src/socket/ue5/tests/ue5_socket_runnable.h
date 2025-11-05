#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformAffinity.h"
#include "CoreMinimal.h"
#include "socket/ue5/ue5_socket.h"
#include "socket/ue5/ue5_websocket.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include <atomic>
#include <memory>

namespace reactormq::socket
{
    /**
     * @brief Test helper that runs a socket in a separate thread.
     * 
     * This runnable wraps a ReactorMQ socket (Ue5Socket or Ue5WebSocket) and continuously
     * calls tick() in a background thread, allowing realistic testing of socket behavior.
     * The thread automatically stops when the socket disconnects or when Stop() is called.
     */
    class Ue5SocketRunnable final : public FRunnable
    {
    public:
        /**
         * @brief Construct a socket runnable from a connection settings pointer.
         * @param settings Connection settings for the socket.
         */
        explicit Ue5SocketRunnable(const mqtt::ConnectionSettingsPtr& settings)
            : m_isStopping(false)
            , m_socket(createSocketFromSettings(settings))
        {
            if (m_socket)
            {
                m_connectConnection = m_socket->getOnConnectCallback().add(
                    [this](const bool wasSuccessful)
                    {
                        if (!wasSuccessful)
                        {
                            m_isStopping.store(true, std::memory_order_release);
                        }
                    });

                m_disconnectConnection = m_socket->getOnDisconnectCallback().add(
                    [this]()
                    {
                        m_isStopping.store(true, std::memory_order_release);
                    });
            }

            m_thread = FRunnableThread::Create(
                this,
                TEXT("Ue5SocketRunnable"),
                0,
                TPri_Normal,
                FPlatformAffinity::GetPoolThreadMask());
        }

        /**
         * @brief Destructor. Stops the thread and cleans up resources.
         */
        virtual ~Ue5SocketRunnable() override
        {
            if (m_thread)
            {
                m_thread->Kill(true);
                delete m_thread;
                m_thread = nullptr;
            }
        }

        /**
         * @brief Get the underlying socket instance.
         * @return Shared pointer to the socket.
         */
        std::shared_ptr<SocketBase> getSocket()
        {
            return m_socket;
        }

        /**
         * @brief Main thread run function. Continuously ticks the socket.
         * @return Exit code (0 for normal completion).
         */
        virtual uint32 Run() override
        {
            while (!m_isStopping.load(std::memory_order_acquire))
            {
                if (m_socket)
                {
                    m_socket->tick();
                }
                FPlatformProcess::SleepNoStats(0.0001f);
            }
            return 0;
        }

        /**
         * @brief Stop the runnable thread and disconnect the socket.
         */
        virtual void Stop() override
        {
            FRunnable::Stop();
            if (m_socket)
            {
                m_socket->disconnect();
            }
            m_isStopping.store(true, std::memory_order_release);
        }

    private:
        /**
         * @brief Create the appropriate socket type based on connection settings.
         * @param settings Connection settings.
         * @return Shared pointer to the created socket.
         */
        static std::shared_ptr<SocketBase> createSocketFromSettings(const mqtt::ConnectionSettingsPtr& settings)
        {
            if (!settings)
            {
                return nullptr;
            }

            const mqtt::ConnectionProtocol protocol = settings->getProtocol();
            
            if (protocol == mqtt::ConnectionProtocol::Ws || protocol == mqtt::ConnectionProtocol::Wss)
            {
#ifdef REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5
                return std::make_shared<Ue5WebSocket>(settings);
#else
                return nullptr;
#endif // REACTORMQ_SOCKET_WITH_WEBSOCKET_UE5
            }
            else
            {
#ifdef REACTORMQ_WITH_FSOCKET_UE5
                return std::make_shared<Ue5Socket>(settings);
#else
                return nullptr;
#endif // REACTORMQ_WITH_FSOCKET_UE5
            }
        }

        /// @brief Atomic flag to signal thread stopping.
        std::atomic<bool> m_isStopping;

        /// @brief The socket being tested.
        std::shared_ptr<SocketBase> m_socket;

        /// @brief The background thread running the socket.
        FRunnableThread* m_thread = nullptr;

        /// @brief Connection handle for connect callback.
        mqtt::Connection m_connectConnection;

        /// @brief Connection handle for disconnect callback.
        mqtt::Connection m_disconnectConnection;
    };
} // namespace reactormq::socket

#endif // WITH_DEV_AUTOMATION_TESTS

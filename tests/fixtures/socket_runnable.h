#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "reactormq/mqtt/connection_settings.h"
#include "socket/native/native_socket.h"

namespace reactormq::tests
{
    /**
     * @brief Test fixture that runs a NativeSocket in a background thread.
     *
     * Continuously calls tick() on the socket at regular intervals.
     * Automatically stops on connection failure or disconnect.
     * Useful for testing socket behavior with realistic async I/O.
     */
    class SocketRunnable final
    {
    public:
        /**
         * @brief Constructor.
         * @param settings Connection settings for the socket.
         */
        explicit SocketRunnable(const mqtt::ConnectionSettingsPtr& settings)
            : m_socket(std::make_shared<socket::NativeSocket>(settings))
              , m_shouldStop(false)
              , m_isRunning(false)
        {
            m_connectHandle = m_socket->getOnConnectCallback().add([this](bool wasSuccessful)
            {
                if (!wasSuccessful)
                {
                    m_shouldStop.store(true, std::memory_order_release);
                }
            });

            m_disconnectHandle = m_socket->getOnDisconnectCallback().add([this]()
            {
                m_shouldStop.store(true, std::memory_order_release);
            });

            m_isRunning.store(true, std::memory_order_release);
            m_thread = std::thread(&SocketRunnable::run, this);
        }

        /**
         * @brief Destructor. Stops the thread and cleans up.
         */
        ~SocketRunnable()
        {
            stop();
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        SocketRunnable(const SocketRunnable&) = delete;

        SocketRunnable& operator=(const SocketRunnable&) = delete;

        SocketRunnable(SocketRunnable&&) = delete;

        SocketRunnable& operator=(SocketRunnable&&) = delete;

        /**
         * @brief Get the socket instance.
         * @return Shared pointer to the socket.
         */
        std::shared_ptr<socket::NativeSocket> getSocket()
        {
            return m_socket;
        }

        /**
         * @brief Stop the background thread.
         */
        void stop()
        {
            if (m_socket)
            {
                m_socket->disconnect();
            }
            m_shouldStop.store(true, std::memory_order_release);
        }

        /**
         * @brief Check if the thread is still running.
         * @return True if running, false otherwise.
         */
        bool isRunning() const
        {
            return m_isRunning.load(std::memory_order_acquire);
        }

    private:
        void run()
        {
            while (!m_shouldStop.load(std::memory_order_acquire))
            {
                if (m_socket)
                {
                    m_socket->tick();
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            m_isRunning.store(false, std::memory_order_release);
        }

        std::shared_ptr<socket::NativeSocket> m_socket;
        std::atomic<bool> m_shouldStop;
        std::atomic<bool> m_isRunning;
        std::thread m_thread;
        mqtt::DelegateHandle m_connectHandle;
        mqtt::DelegateHandle m_disconnectHandle;
    };
} // namespace reactormq::tests

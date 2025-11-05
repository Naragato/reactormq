#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "socket_tickable.h"
#include "reactormq/mqtt/connection_settings.h"
#include "reactormq/mqtt/delegates.h"

namespace reactormq::socket
{
    class SocketBase;

    /**
     * @brief Shared pointer type for socket instances.
     */
    using SocketPtr = std::shared_ptr<SocketBase>;

    /**
     * @brief Weak pointer type for socket instances.
     */
    using SocketWeakPtr = std::weak_ptr<SocketBase>;

    /**
     * @brief Multicast delegate for connection events.
     * 
     * Use `.add()` to subscribe callbacks, which returns a Connection handle
     * that can be used to unsubscribe later.
     * 
     * @param wasSuccessful True if the connection was successful, false otherwise.
     */
    using OnConnectCallback = mqtt::MulticastDelegate<void(bool wasSuccessful)>;

    /**
     * @brief Multicast delegate for disconnection events.
     * 
     * Use `.add()` to subscribe callbacks, which returns a Connection handle
     * that can be used to unsubscribe later.
     */
    using OnDisconnectCallback = mqtt::MulticastDelegate<void()>;

    /**
     * @brief Multicast delegate for data received events.
     * 
     * Use `.add()` to subscribe callbacks, which returns a Connection handle
     * that can be used to unsubscribe later.
     * 
     * @param data Pointer to the received data buffer.
     * @param size Size of the received data in bytes.
     */
    using OnDataReceivedCallback = mqtt::MulticastDelegate<void(const uint8_t* data, uint32_t size)>;

    [[nodiscard]] SocketPtr CreateSocket(const mqtt::ConnectionSettingsPtr& settings);

    /**
     * @brief Abstract base class for socket implementations.
     * 
     * Provides the interface for socket operations including connect, disconnect,
     * send, and receive. This interface is platform-agnostic and does not depend
     * on Unreal Engine types.
     */
    class SocketBase : public ISocketTickable
    {
    public:
        /**
         * @brief Constructor.
         * @param settings Connection settings containing host, port, protocol, and verification options.
         */
        explicit SocketBase(mqtt::ConnectionSettingsPtr settings) : m_settings(std::move(settings))
        {
        }

        /**
         * @brief Virtual destructor.
         */
        ~SocketBase() override = default;

        /**
         * @brief Connect to the remote host.
         */
        virtual void connect() = 0;

        /**
         * @brief Disconnect from the remote host.
         */
        virtual void disconnect() = 0;

        /**
         * @brief Close the socket connection.
         * @param code The close code (default: 1000 for normal closure).
         * @param reason The reason for closing the connection.
         */
        virtual void close(int32_t code, const std::string& reason) = 0;

        /**
         * @brief Convenience close required by ISocketTickable.
         */
        void close() override { close(1000, {}); }

        /**
         * @brief Check if the socket is currently connected.
         * @return True if connected, false otherwise.
         */
        [[nodiscard]] virtual bool isConnected() const = 0;

        /**
         * @brief Return a stable identifier of the concrete implementation for diagnostics/tests.
         * Due to RTTI
         * @return Implementation identifier string (e.g., "NativeSocket", "Ue5Socket").
         */
        virtual const char* getImplementationId() const
        {
            return "SocketBase";
        }

        /**
         * @brief Send raw data through the socket.
         * @param data Pointer to the data buffer to send.
         * @param size Size of the data in bytes.
         */
        virtual void send(const uint8_t* data, uint32_t size) = 0;

        /**
         * @brief Get the connection events delegate.
         * @return Reference to the OnConnectCallback delegate.
         */
        virtual OnConnectCallback& getOnConnectCallback() = 0;

        /**
         * @brief Get the disconnection events delegate.
         * @return Reference to the OnDisconnectCallback delegate.
         */
        virtual OnDisconnectCallback& getOnDisconnectCallback() = 0;

        /**
         * @brief Get the data received events delegate.
         * @return Reference to the OnDataReceivedCallback delegate.
         */
        virtual OnDataReceivedCallback& getOnDataReceivedCallback() = 0;

    protected:
        /**
         * @brief Shared MQTT packet parser using the buffer and read offset.
         *
         * Parses MQTT Remaining Length (1–4 bytes) and dispatches complete packets via invokeOnDataReceived.
         * Compacts the buffer when the read offset is at least 256 KiB and at least 75% of the buffer size.
         * Returns false if a packet exceeds the configured maxPacketSize.
         *
         * This method is internally synchronized: it acquires m_socketMutex while parsing and compacting
         * the internal buffer. Callers do not need to lock around this call.
         *
         * @return True if parsing succeeded, false if an oversized packet was encountered.
         */
        bool readPacketsFromBuffer()
        {
            std::vector<std::vector<uint8_t> > packetsToDispatch;
            {
                std::scoped_lock lock(m_socketMutex);

                const uint32_t maxPacketSize = (nullptr != m_settings ? m_settings->getMaxPacketSize() : 268435455u);

                size_t available = m_dataBuffer.size() - m_dataBufferReadOffset;

                while (available > 1)
                {
                    const uint8_t* base = m_dataBuffer.data() + m_dataBufferReadOffset;
                    uint32_t remainingLength = 0;
                    uint32_t multiplier = 1;
                    size_t index = 1;
                    bool haveRemainingLength = false;

                    for (; index < 5; ++index)
                    {
                        if (index >= available)
                        {
                            break;
                        }

                        const uint8_t encodedByte = base[index];
                        remainingLength += static_cast<uint32_t>(encodedByte & 127u) * multiplier;
                        multiplier *= 128u;

                        if ((encodedByte & 128u) == 0)
                        {
                            haveRemainingLength = true;
                            ++index;
                            break;
                        }
                    }

                    if (!haveRemainingLength)
                    {
                        break;
                    }

                    if (remainingLength > maxPacketSize)
                    {
                        return false;
                    }

                    const size_t fixedHeaderSize = index;
                    const size_t totalPacketSize = fixedHeaderSize + remainingLength;

                    if (available < totalPacketSize)
                    {
                        break;
                    }

                    packetsToDispatch.emplace_back(base, base + totalPacketSize);

                    m_dataBufferReadOffset += totalPacketSize;
                    available -= totalPacketSize;
                }

                if (m_dataBufferReadOffset > 0)
                {
                    if (m_dataBufferReadOffset == m_dataBuffer.size())
                    {
                        m_dataBuffer.clear();
                        m_dataBufferReadOffset = 0;
                    } else
                    {
                        constexpr size_t compactMinBytes = 256 * 1024;
                        constexpr float compactFraction = 0.75f;
                        const auto thresholdByFraction = static_cast<size_t>(
                            compactFraction * static_cast<float>(m_dataBuffer.size()));

                        if (m_dataBufferReadOffset >= compactMinBytes && m_dataBufferReadOffset >= thresholdByFraction)
                        {
                            m_dataBuffer.erase(m_dataBuffer.begin(),
                                               m_dataBuffer.begin() + static_cast<std::ptrdiff_t>(
                                                   m_dataBufferReadOffset));
                            m_dataBufferReadOffset = 0;
                        }
                    }
                }
            }

            for (const auto& p: packetsToDispatch)
            {
                invokeOnDataReceived(p.data(), static_cast<uint32_t>(p.size()));
            }

            return true;
        }

        /**
         * @brief Invoke the connection callback delegate.
         * @param wasSuccessful True if the connection was successful.
         */
        void invokeOnConnect(const bool wasSuccessful)
        {
            getOnConnectCallback().broadcast(wasSuccessful);
        }

        /**
         * @brief Invoke the disconnection callback delegate.
         */
        void invokeOnDisconnect()
        {
            getOnDisconnectCallback().broadcast();
        }

        /**
         * @brief Invoke the data received callback delegate.
         * @param data Pointer to the received data buffer.
         * @param size Size of the received data in bytes.
         */
        void invokeOnDataReceived(const uint8_t* data, const uint32_t size)
        {
            getOnDataReceivedCallback().broadcast(data, size);
        }

        /**
         * @brief Mutex for protecting socket operations.
         */
        mutable std::mutex m_socketMutex;

        /**
         * @brief Internal data buffer for accumulating received data.
         */
        std::vector<uint8_t> m_dataBuffer;

        /**
         * @brief Offset into the data buffer indicating how many bytes have been consumed.
         */
        size_t m_dataBufferReadOffset = 0;

        /**
         * @brief Connection settings used to control parsing and limits.
         */
        mqtt::ConnectionSettingsPtr m_settings;
    };
} // namespace reactormq::socket

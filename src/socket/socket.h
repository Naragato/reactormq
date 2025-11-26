//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/mqtt/connection_settings.h"
#include "reactormq/mqtt/delegates.h"

#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace reactormq::socket
{
    class Socket;

    using SocketPtr = std::shared_ptr<Socket>;

    using SocketWeakPtr = std::weak_ptr<Socket>;

    /**
     * @brief Event fired after a connection attempt.
     * @param wasSuccessful True if the connection was established.
     */
    using OnConnectCallback = mqtt::MulticastDelegate<void(bool wasSuccessful)>;

    /// @brief Event fired when the connection closes.
    using OnDisconnectCallback = mqtt::MulticastDelegate<void()>;

    /**
     * @brief Event fired when raw bytes arrive from the peer.
     * @param data Pointer to the received buffer.
     * @param size Number of bytes in the buffer.
     */
    using OnDataReceivedCallback = mqtt::MulticastDelegate<void(const uint8_t* data, uint32_t size)>;

    [[nodiscard]] SocketPtr CreateSocket(const mqtt::ConnectionSettingsPtr& settings);

    /**
     * @brief Platform-agnostic socket interface used by the MQTT client.
     * Provides connection control, sending, and event callbacks.
     *
     */
    class Socket
    {
    public:
        /**
         * @brief Create a socket bound to the given connection settings.
         * @param settings Connection parameters used by the implementation.
         *
         */
        explicit Socket(mqtt::ConnectionSettingsPtr settings)
            : m_settings(std::move(settings))
        {
        }

        virtual ~Socket() = default;

        /**
         * @brief Update the object state. Called periodically.
         */
        virtual void tick() = 0;

        /// @brief Initiate a connection to the remote host.
        virtual void connect() = 0;

        /// @brief Terminate the connection to the remote host.
        virtual void disconnect() = 0;

        /**
         * @brief Close the connection with an application-defined code and reason.
         * @param code Close code; 1000 means normal closure.
         * @param reason Human-readable reason for closing.
         */
        virtual void close(int32_t code, const std::string& reason) = 0;

        /// @brief Close with a normal code and empty reason.
        virtual void close()
        {
            close(1000, {});
        }

        /**
         * @brief Report whether the socket is currently connected.
         * @return True if a connection is active.
         *
         */
        [[nodiscard]] virtual bool isConnected() const = 0;

        /**
         * @brief Return a stable identifier for the concrete implementation (for diagnostics/tests).
         * @return Implementation name, for example "SocketBase" or a derived type.
         *
         */
        [[nodiscard]] virtual const char* getImplementationId() const
        {
            return "SocketBase";
        }

        /**
         * @brief Send raw bytes over the connection.
         * @param data Buffer to transmit.
         * @param size Number of bytes to send.
         */
        virtual void send(const uint8_t* data, uint32_t size) = 0;

        /**
         * @brief Send raw bytes over the connection.
         * @param data Buffer to transmit.
         * @param size Number of bytes to send.
         */
        virtual void send(const std::byte* data, const std::uint32_t size)
        {
            send(std::span{ data, size });
        }

        /**
         * @brief Send raw bytes over the connection.
         * @param data Buffer to transmit.
         */
        virtual void send(const std::span<const std::byte> data)
        {
            send(reinterpret_cast<const uint8_t*>(data.data()), static_cast<uint32_t>(data.size()));
        }

        /// @brief Access the connection event.
        virtual OnConnectCallback& getOnConnectCallback() = 0;

        /// @brief Access the disconnection event.
        virtual OnDisconnectCallback& getOnDisconnectCallback() = 0;

        /// @brief Access the data-received event.
        virtual OnDataReceivedCallback& getOnDataReceivedCallback() = 0;

    protected:
        /**
         * @brief processes the packet data received from the socket and dispatches to callbacks via
         * `readPacketsFromBuffer`
         * @param data incoming data ptr
         * @param size length / dize of data
         * @return false on error
         */
        bool processPacketData(const uint8_t* data, size_t size);

        /**
         * @brief Parse buffered bytes into complete MQTT packets and emit data callbacks.
         * @return True if parsing succeeded; false if a packet exceeds the configured maximum size.
         *
         */
        bool readPacketsFromBuffer()
        {
            std::vector<std::vector<uint8_t>> packetsToDispatch;
            {
                const uint32_t maxPacketSize = nullptr != m_settings ? m_settings->getMaxPacketSize() : 268435455u;

                size_t available = m_dataBuffer.size() - m_dataBufferReadOffset;

                bool keepParsing = true;

                while (available > 1U && keepParsing == true)
                {
                    const uint8_t* base = m_dataBuffer.data() + m_dataBufferReadOffset;
                    uint32_t remainingLength = 0U;
                    uint32_t multiplier = 1U;
                    size_t index = 1U;
                    bool haveRemainingLength = false;
                    while (index < 5U && index < available && haveRemainingLength == false)
                    {
                        constexpr uint8_t remainingLengthValueMask = 0x7FU;

                        const uint8_t encodedByte = base[index];

                        remainingLength += static_cast<uint32_t>(encodedByte & remainingLengthValueMask) * multiplier;
                        multiplier *= 128U;

                        ++index;

                        if (constexpr uint8_t remainingLengthContinueBit = 0x80u; (encodedByte & remainingLengthContinueBit) == 0U)
                        {
                            haveRemainingLength = true;
                        }
                    }

                    if (haveRemainingLength == false)
                    {
                        keepParsing = false; // not enough bytes to finish remaining length field
                    }
                    else if (remainingLength > maxPacketSize)
                    {
                        return false;
                    }
                    else
                    {
                        const size_t fixedHeaderSize = index;
                        const size_t remainingLengthSz = remainingLength;
                        const size_t totalPacketSize = fixedHeaderSize + remainingLengthSz;

                        if (available < totalPacketSize)
                        {
                            keepParsing = false; // incomplete packet in buffer
                        }
                        else
                        {
                            packetsToDispatch.emplace_back(base, base + totalPacketSize);

                            m_dataBufferReadOffset += totalPacketSize;
                            available -= totalPacketSize;
                        }
                    }
                }

                if (m_dataBufferReadOffset > 0)
                {
                    if (m_dataBufferReadOffset == m_dataBuffer.size())
                    {
                        m_dataBuffer.clear();
                        m_dataBufferReadOffset = 0;
                    }
                    else
                    {
                        constexpr size_t compactMinBytes = 256 * 1024;
                        constexpr float compactFraction = 0.75f;
                        const auto thresholdByFraction = static_cast<size_t>(compactFraction * static_cast<float>(m_dataBuffer.size()));

                        if (m_dataBufferReadOffset >= compactMinBytes && m_dataBufferReadOffset >= thresholdByFraction)
                        {
                            m_dataBuffer
                                .erase(m_dataBuffer.begin(), m_dataBuffer.begin() + static_cast<std::ptrdiff_t>(m_dataBufferReadOffset));
                            m_dataBufferReadOffset = 0;
                        }
                    }
                }
            }

            for (const auto& p : packetsToDispatch)
            {
                invokeOnDataReceived(p.data(), static_cast<uint32_t>(p.size()));
            }

            return true;
        }

        /// @brief Internal helper to notify listeners after a connection attempt.
        void invokeOnConnect(const bool wasSuccessful)
        {
            getOnConnectCallback().broadcast(wasSuccessful);
        }

        /// @brief Internal helper to notify listeners on disconnect.
        void invokeOnDisconnect()
        {
            getOnDisconnectCallback().broadcast();
        }

        /// @brief Internal helper to deliver received bytes to listeners.
        void invokeOnDataReceived(const uint8_t* data, const uint32_t size)
        {
            getOnDataReceivedCallback().broadcast(data, size);
        }

        /// @return get the settings ptr
        mqtt::ConnectionSettingsPtr getSettings() const
        {
            return m_settings;
        }

    private:
        std::vector<uint8_t> m_dataBuffer; ///< Internal buffer for accumulating received packet bytes.
        size_t m_dataBufferReadOffset = 0; ///< Offset into the buffer for already-consumed bytes.

        mqtt::ConnectionSettingsPtr m_settings;
    };
} // namespace reactormq::socket
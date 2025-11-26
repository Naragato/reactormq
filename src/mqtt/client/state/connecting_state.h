//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "state.h"

#include <chrono>
#include <future>
#include <optional>

namespace reactormq::mqtt::packets
{
    template<ProtocolVersion V>
    class ConnAck;
}

namespace reactormq::mqtt::client
{
    /**
     * @brief State representing a client attempting to connect to the broker.
     *
     * Initiates the socket connection, sends CONNECT, and waits for CONNACK.
     * Handles connection timeout and backoff on failure.
     */
    class ConnectingState final : public IState
    {
    public:
        /**
         * @brief Construct a ConnectingState.
         * @param cleanSession Whether to start a clean session vs. resume.
         * @param promise Promise completed when the connect attempt finishes.
         */
        explicit ConnectingState(bool cleanSession, std::promise<Result<void>> promise);

        ~ConnectingState() override = default;

        StateTransition onEnter(Context& context) override;

        void onExit(Context& context) override;

        StateTransition handleCommand(Context& context, Command& command) override;

        StateTransition onSocketConnected(Context& context) override;

        StateTransition onSocketDisconnected(Context& context) override;

        StateTransition onDataReceived(Context& context, const uint8_t* data, uint32_t size) override;

        StateTransition onTick(Context& context) override;

        [[nodiscard]] const char* getStateName() const override;

    private:
        StateTransition handleConnAck(Context& context, const packets::IControlPacket& packet);
        static void assignClientId(Context& context, const packets::ConnAck<packets::ProtocolVersion::V5>& connAck);

        bool m_cleanSession;
        std::optional<std::promise<Result<void>>> m_promise;
        std::optional<std::chrono::steady_clock::time_point> m_handshakeDeadline;
    };
} // namespace reactormq::mqtt::client
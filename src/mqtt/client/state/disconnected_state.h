//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/client/backoff.h"
#include "state.h"

#include <chrono>
#include <optional>

namespace reactormq::mqtt::client
{
    /**
     * @brief State representing a disconnected client.
     * Accepts connectAsync to initiate a connection.
     * Can auto-reconnect with exponential backoff after an unexpected drop.
     */
    class DisconnectedState final : public IState
    {
    public:
        /**
         * @brief Construct a DisconnectedState.
         * @param wasGracefulDisconnect True if the client initiated the disconnect; false if the connection was lost.
         */
        explicit DisconnectedState(bool wasGracefulDisconnect = false);

        ~DisconnectedState() override = default;

        StateTransition onEnter(Context& context) override;

        void onExit(Context& context) override;

        StateTransition handleCommand(Context& context, Command& command) override;

        StateTransition onSocketConnected(Context& context) override;

        StateTransition onSocketDisconnected(Context& context) override;

        StateTransition onDataReceived(Context& context, const uint8_t* data, uint32_t size) override;

        StateTransition onTick(Context& context) override;

        [[nodiscard]] const char* getStateName() const override
        {
            return "Disconnected";
        }

    private:
        bool m_wasGracefulDisconnect = false;
        std::optional<std::chrono::steady_clock::time_point> m_nextRetryTime;
        std::optional<BackoffCalculator> m_backoffCalculator;
    };
} // namespace reactormq::mqtt::client
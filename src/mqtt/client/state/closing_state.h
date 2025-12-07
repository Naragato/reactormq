//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/mqtt/result.h"
#include "state.h"

#include <future>
#include <optional>

namespace reactormq::mqtt::client
{
    /**
     * @brief State representing a client performing a graceful disconnect.
     * Drains the outbound queue, sends DISCONNECT, and closes the socket.
     * Transitions to Disconnected when complete or on timeout.
     */
    class ClosingState final : public IState
    {
    public:
        /**
         * @brief Construct a ClosingState.
         * @param promise Promise fulfilled when the disconnect completes.
         */
        explicit ClosingState(std::promise<Result<void>> promise);

        ~ClosingState() override = default;

        StateTransition onEnter(Context& context) override;

        void onExit(Context& context) override;

        StateTransition handleCommand(Context& context, Command& command) override;

        StateTransition onSocketConnected(Context& context) override;

        StateTransition onSocketDisconnected(Context& context) override;

        StateTransition onDataReceived(Context& context, const uint8_t* data, uint32_t size) override;

        StateTransition onTick(Context& context) override;

        [[nodiscard]] const char* getStateName() const override
        {
            return "Closing";
        }

        [[nodiscard]] StateId getStateId() const override
        {
            return StateId::Closing;
        }

    private:
        std::optional<std::promise<Result<void>>> m_promise;
        std::chrono::steady_clock::time_point m_entryTime;
    };
} // namespace reactormq::mqtt::client
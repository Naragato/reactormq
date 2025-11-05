#pragma once

#include "state.h"
#include "reactormq/mqtt/result.h"

#include <future>
#include <optional>

namespace reactormq::mqtt::client
{
    /**
     * @brief State representing a client in the process of gracefully disconnecting.
     * 
     * Drains outbound queue, sends DISCONNECT packet, closes socket.
     * Transitions to Disconnected when complete or on timeout.
     */
    class ClosingState : public IState
    {
    public:
        /**
         * @brief Constructor.
         * @param promise Promise to complete when disconnect finishes.
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

        const char* getStateName() const override
        {
            return "Closing";
        }

    private:
        /**
         * @brief Promise to complete when disconnect finishes.
         */
        std::optional<std::promise<Result<void>>> m_promise;

        /**
         * @brief Time when we entered this state (for timeout tracking).
         */
        std::chrono::steady_clock::time_point m_entryTime;
    };

} // namespace reactormq::mqtt::client

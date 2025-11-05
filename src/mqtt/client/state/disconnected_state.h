#pragma once

#include "state.h"
#include "mqtt/client/backoff.h"

#include <chrono>
#include <optional>

namespace reactormq::mqtt::client
{
    /**
     * @brief State representing a disconnected client.
     * 
     * Accepts connectAsync commands to initiate connection.
     * Supports automatic reconnection with exponential backoff after unexpected disconnections.
     */
    class DisconnectedState : public IState
    {
    public:
        /**
         * @brief Constructor.
         * @param wasGracefulDisconnect True if disconnect was initiated by client (graceful),
         *                              false if connection was lost unexpectedly.
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

        const char* getStateName() const override
        {
            return "Disconnected";
        }

    private:
        /**
         * @brief True if disconnect was graceful (user-initiated), false if unexpected.
         */
        bool m_wasGracefulDisconnect = false;

        /**
         * @brief Time point when next reconnection attempt should be made.
         */
        std::optional<std::chrono::steady_clock::time_point> m_nextRetryTime;

        /**
         * @brief Backoff calculator for auto-reconnect (initialized if auto-reconnect is enabled).
         */
        std::optional<BackoffCalculator> m_backoffCalculator;
    };

} // namespace reactormq::mqtt::client

#pragma once

#include "state.h"
#include <future>
#include <optional>

namespace reactormq::mqtt::client
{
    /**
     * @brief State representing a client attempting to connect to the broker.
     * 
     * Initiates socket connection, sends CONNECT packet, waits for CONNACK.
     * Handles connection timeout and backoff on failure.
     */
    class ConnectingState : public IState
    {
    public:
        explicit ConnectingState(bool cleanSession, std::promise<Result<void>> promise);
        ~ConnectingState() override = default;

        StateTransition onEnter(Context& context) override;
        void onExit(Context& context) override;
        StateTransition handleCommand(Context& context, Command& command) override;
        StateTransition onSocketConnected(Context& context) override;
        StateTransition onSocketDisconnected(Context& context) override;
        StateTransition onDataReceived(Context& context, const uint8_t* data, uint32_t size) override;
        StateTransition onTick(Context& context) override;

        const char* getStateName() const override
        {
            return "Connecting";
        }

    private:
        bool m_cleanSession;
        std::optional<std::promise<Result<void>>> m_promise;
    };

} // namespace reactormq::mqtt::client

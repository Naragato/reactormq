#pragma once

#include "state.h"

namespace reactormq::mqtt::client
{
    /**
     * @brief State representing a connected client ready for MQTT operations.
     * 
     * Handles publish, subscribe, unsubscribe operations.
     * Manages keepalive (PINGREQ/PINGRESP), ACK tracking for QoS 1/2.
     */
    class ReadyState : public IState
    {
    public:
        ReadyState() = default;
        ~ReadyState() override = default;

        StateTransition onEnter(Context& context) override;
        void onExit(Context& context) override;
        StateTransition handleCommand(Context& context, Command& command) override;
        StateTransition onSocketConnected(Context& context) override;
        StateTransition onSocketDisconnected(Context& context) override;
        StateTransition onDataReceived(Context& context, const uint8_t* data, uint32_t size) override;
        StateTransition onTick(Context& context) override;

        const char* getStateName() const override
        {
            return "Ready";
        }
    };

} // namespace reactormq::mqtt::client

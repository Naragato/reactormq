//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <memory>

#include "mqtt/client/command.h"
#include "mqtt/client/context.h"
#include "mqtt/client/state/state_transition.h"

namespace reactormq::mqtt::client
{
    class IState;

    using StatePtr = std::unique_ptr<IState>;

    /**
     * @brief Interface for connection lifecycle states.
     *
     * Each state handles commands, socket events, and timers appropriate for that phase.
     * States are owned by the Reactor and accessed only on the reactor thread.
     */
    class IState
    {
    public:
        virtual ~IState() = default;

        /**
         * @brief Called when entering this state.
         * @param context Shared context.
         * @return Optional state transition.
         */
        virtual StateTransition onEnter(Context& context) = 0;

        /**
         * @brief Called when exiting this state.
         * @param context Shared context.
         */
        virtual void onExit(Context& context) = 0;

        /**
         * @brief Handle a command from the client API.
         * @param context Shared context.
         * @param command The command to handle.
         * @return Optional state transition.
         */
        virtual StateTransition handleCommand(Context& context, Command& command) = 0;

        /**
         * @brief Called when the socket reports a successful connection.
         * @param context Shared context.
         * @return Optional state transition.
         */
        virtual StateTransition onSocketConnected(Context& context) = 0;

        /**
         * @brief Called when the socket reports a disconnection.
         * @param context Shared context.
         * @return Optional state transition.
         */
        virtual StateTransition onSocketDisconnected(Context& context) = 0;

        /**
         * @brief Called when data is received from the socket.
         * @param context Shared context.
         * @param data Pointer to received data.
         * @param size Size of received data in bytes.
         * @return Optional state transition.
         */
        virtual StateTransition onDataReceived(Context& context, const uint8_t* data, uint32_t size) = 0;

        /**
         * @brief Called periodically to allow the state to perform time-based operations.
         * @param context Shared context.
         * @return Optional state transition.
         */
        virtual StateTransition onTick(Context& context) = 0;

        /**
         * @brief Get a human-readable name for this state (for debugging/logging).
         * @return State name.
         */
        [[nodiscard]] virtual const char* getStateName() const = 0;
    };
} // namespace reactormq::mqtt::client
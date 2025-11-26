//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/client/command.h"
#include "mqtt/client/context.h"
#include "mqtt/client/state/state.h"
#include "reactormq/mqtt/connection_settings.h"

#include <deque>
#include <memory>
#include <mutex>

namespace reactormq::mqtt::client
{
    /**
     * @brief Reactor that drives the event loop, state machine, and command queue.
     * Owns the current State, Context, and command queue.
     * Dispatches events and commands to the active state.
     */
    class Reactor : public std::enable_shared_from_this<Reactor>
    {
    public:
        /**
         * @brief Constructor.
         * @param settings Connection settings for the MQTT client.
         */
        explicit Reactor(const ConnectionSettingsPtr& settings);

        ~Reactor();

        /**
         * @brief Enqueue a command for execution on the reactor thread.
         * @param command The command to enqueue.
         */
        void enqueueCommand(Command command);

        /**
         * @brief Tick the reactor (one iteration of the event loop).
         * Processes command queue, ticks current state, and ticks socket.
         */
        void tick();

        /**
         * @brief Get the name of the current state.
         * @return State name string.
         */
        [[nodiscard]] const char* getCurrentStateName() const;

        /**
         * @brief Check if the reactor is in the connected state.
         * @return True if connected, false otherwise.
         */
        [[nodiscard]] bool isConnected() const;

        /**
         * @brief Get the shared context.
         * @return Reference to the context.
         */
        Context& getContext()
        {
            return m_context;
        }

    private:
        /**
         * @brief Transition to a new state.
         * @param toState The new state to transition to.
         */
        void transitionToState(StatePtr toState);

        /**
         * @brief Process all commands in the command queue.
         */
        void processCommandQueue();

        /**
         * @brief Set up socket callbacks for the current socket.
         */
        void setupSocketCallbacks();

        Context m_context;
        StatePtr m_currentState;
        std::deque<Command> m_commandQueue;
        std::mutex m_commandQueueMutex;
        DelegateHandle m_socketReplacedHandle;
    };
} // namespace reactormq::mqtt::client
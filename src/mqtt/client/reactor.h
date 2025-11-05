#pragma once

#include "mqtt/client/command.h"
#include "mqtt/client/context.h"
#include "mqtt/client/state/state.h"
#include "reactormq/mqtt/connection_settings.h"

#include <deque>
#include <memory>
#include <mutex>
#include <vector>

namespace reactormq::mqtt::client
{
    /**
     * @brief Reactor managing the event loop, state machine, and command queue.
     * 
     * Owns the current State, Context, and command queue.
     * Dispatches events and commands to the current state.
     */
    class Reactor : public std::enable_shared_from_this<Reactor>
    {
    public:
        /**
         * @brief Constructor.
         * @param settings Connection settings for the MQTT client.
         */
        explicit Reactor(ConnectionSettingsPtr settings);

        /**
         * @brief Destructor.
         */
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
         * @brief Check if the reactor is in the connected (Ready) state.
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
         * @param newState The new state to transition to.
         */
        void transitionToState(StatePtr newState);

        /**
         * @brief Process all commands in the command queue.
         */
        void processCommandQueue();

        /**
         * @brief Set up socket callbacks for the current socket.
         */
        void setupSocketCallbacks();

        /**
         * @brief Shared context containing socket, settings, and callbacks.
         */
        Context m_context;

        /**
         * @brief Current state of the state machine.
         */
        StatePtr m_currentState;

        /**
         * @brief Command queue for async API calls.
         */
        std::deque<Command> m_commandQueue;

        /**
         * @brief Mutex protecting the command queue.
         */
        std::mutex m_commandQueueMutex;

        /**
         * @brief Flag indicating whether socket callbacks have been set up.
         */
        bool m_socketCallbacksSetup = false;
    };

} // namespace reactormq::mqtt::client

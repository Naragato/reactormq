//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/client/reactor.h"

#include "mqtt/client/state/disconnected_state.h"
#include "socket/socket.h"
#include "util/logging/logging.h"

#include <cstring>

namespace reactormq::mqtt::client
{
    Reactor::Reactor(const ConnectionSettingsPtr& settings)
        : m_context(settings)
        , m_currentState(std::make_unique<DisconnectedState>())
    {
        REACTORMQ_LOG(
            logging::LogLevel::Info,
            "Reactor::Reactor() created (initialState=%s)",
            m_currentState ? m_currentState->getStateName() : "None");

        m_socketReplacedHandle = m_context.getOnSocketReplaced().add(
            [this]
            {
                REACTORMQ_LOG(logging::LogLevel::Debug, "Reactor socket replaced; rebinding callbacks");
                setupSocketCallbacks();
            });

        if (m_currentState)
        {
            m_currentState->onEnter(m_context);
        }
    }

    Reactor::~Reactor()
    {
        REACTORMQ_LOG(
            logging::LogLevel::Info,
            "Reactor::~Reactor() (currentState=%s)",
            nullptr != m_currentState ? m_currentState->getStateName() : "None");

        m_socketReplacedHandle.disconnect();

        if (m_currentState)
        {
            m_currentState->onExit(m_context);
        }
    }

    void Reactor::enqueueCommand(Command command)
    {
        {
            std::scoped_lock lock(m_commandQueueMutex);
            m_commandQueue.push_back(std::move(command));

            REACTORMQ_LOG(logging::LogLevel::Debug, "Reactor::enqueueCommand() queued command (queueSize=%zu)", m_commandQueue.size());
        }
    }

    void Reactor::tick()
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "Reactor::tick() (state=%s) (", m_currentState ? m_currentState->getStateName() : "None");

        processCommandQueue();

        if (m_currentState)
        {
            auto [newState] = m_currentState->onTick(m_context);
            if (newState.has_value())
            {
                transitionToState(std::move(newState.value()));
            }
        }

        if (const auto sock = m_context.getSocket())
        {
            sock->tick();
        }
    }

    const char* Reactor::getCurrentStateName() const
    {
        const char* name = m_currentState ? m_currentState->getStateName() : "None";
        REACTORMQ_LOG(logging::LogLevel::Trace, "Reactor::getCurrentStateName() -> %s", name);
        return name;
    }

    bool Reactor::isConnected() const
    {
        if (!m_currentState)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "Reactor::isConnected() -> false (no current state)");
            return false;
        }

        const bool connected = (std::strcmp(m_currentState->getStateName(), "Ready") == 0);
        REACTORMQ_LOG(
            logging::LogLevel::Trace,
            "Reactor::isConnected() -> %s (state=%s)",
            connected ? "true" : "false",
            m_currentState->getStateName());
        return connected;
    }

    void Reactor::transitionToState(StatePtr toState)
    {
        if (!toState)
        {
            REACTORMQ_LOG(logging::LogLevel::Warn, "Reactor::transitionToState() called with null target state");
            return;
        }

        const char* fromName = m_currentState ? m_currentState->getStateName() : "None";
        const char* toName = toState->getStateName();

        REACTORMQ_LOG(logging::LogLevel::Info, "Reactor::transitionToState() %s -> %s", fromName, toName);

        if (m_currentState)
        {
            m_currentState->onExit(m_context);
        }

        m_currentState = std::move(toState);

        auto [newState] = m_currentState->onEnter(m_context);

        if (newState.has_value())
        {
            REACTORMQ_LOG(
                logging::LogLevel::Debug,
                "Reactor::transitionToState() chaining transition from state=%s",
                m_currentState ? m_currentState->getStateName() : "None");
            transitionToState(std::move(newState.value()));
        }
    }

    void Reactor::processCommandQueue()
    {
        std::deque<Command> commands;
        {
            std::scoped_lock lock(m_commandQueueMutex);
            commands.swap(m_commandQueue);
        }

        if (!commands.empty())
        {
            REACTORMQ_LOG(
                logging::LogLevel::Debug,
                "Reactor::processCommandQueue() processing %zu command(s) (state=%s)",
                commands.size(),
                m_currentState ? m_currentState->getStateName() : "None");
        }

        for (auto& cmd : commands)
        {
            if (!m_currentState)
            {
                REACTORMQ_LOG(logging::LogLevel::Warn, "Reactor::processCommandQueue() command skipped (no current state)");
                break;
            }

            auto [newState] = m_currentState->handleCommand(m_context, cmd);
            if (newState.has_value())
            {
                transitionToState(std::move(newState.value()));
            }
        }
    }

    void Reactor::setupSocketCallbacks()
    {
        const auto sock = m_context.getSocket();
        if (!sock)
        {
            REACTORMQ_LOG(logging::LogLevel::Trace, "Reactor::setupSocketCallbacks() no socket available");
            return;
        }

        REACTORMQ_LOG(logging::LogLevel::Debug, "Reactor::setupSocketCallbacks() installing callbacks");

        sock->getOnConnectCallback().add(
            [this](const bool wasSuccessful)
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Info,
                    "Reactor socket onConnect callback (success=%s, state=%s)",
                    wasSuccessful ? "true" : "false",
                    m_currentState ? m_currentState->getStateName() : "None");

                if (nullptr == m_currentState)
                {
                    return;
                }

                auto [newState]
                    = wasSuccessful ? m_currentState->onSocketConnected(m_context) : m_currentState->onSocketDisconnected(m_context);

                if (newState.has_value())
                {
                    transitionToState(std::move(newState.value()));
                }
            });

        sock->getOnDisconnectCallback().add(
            [this]
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Info,
                    "Reactor socket onDisconnect callback (state=%s)",
                    m_currentState ? m_currentState->getStateName() : "None");

                if (!m_currentState)
                {
                    return;
                }

                auto [newState] = m_currentState->onSocketDisconnected(m_context);
                if (newState.has_value())
                {
                    transitionToState(std::move(newState.value()));
                }

                m_context.invokeCallback(
                    [&ctx = m_context]
                    {
                        REACTORMQ_LOG(logging::LogLevel::Debug, "Reactor invoking user onDisconnect callback");
                        ctx.getOnDisconnect().broadcast(false);
                    });
            });

        sock->getOnDataReceivedCallback().add(
            [this](const uint8_t* data, const uint32_t size)
            {
                REACTORMQ_LOG(
                    logging::LogLevel::Trace,
                    "Reactor socket onDataReceived callback (size=%u, state=%s)",
                    size,
                    m_currentState ? m_currentState->getStateName() : "None");

                if (!m_currentState)
                {
                    return;
                }

                auto [newState] = m_currentState->onDataReceived(m_context, data, size);
                if (newState.has_value())
                {
                    transitionToState(std::move(newState.value()));
                }
            });

        REACTORMQ_LOG(logging::LogLevel::Debug, "Reactor::setupSocketCallbacks() completed");
    }
} // namespace reactormq::mqtt::client
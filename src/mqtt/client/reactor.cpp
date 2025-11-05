#include "mqtt/client/reactor.h"
#include "mqtt/client/state/disconnected_state.h"
#include "socket/socket_base.h"
#include <cstring>

namespace reactormq::mqtt::client
{
    Reactor::Reactor(ConnectionSettingsPtr settings)
        : m_context(std::move(settings)), m_currentState(std::make_unique<DisconnectedState>())
    {
        if (m_currentState)
        {
            m_currentState->onEnter(m_context);
        }
    }

    Reactor::~Reactor()
    {
        if (m_currentState)
        {
            m_currentState->onExit(m_context);
        }
    }

    void Reactor::enqueueCommand(Command command)
    {
        std::scoped_lock lock(m_commandQueueMutex);
        m_commandQueue.push_back(std::move(command));
    }

    void Reactor::tick()
    {
        setupSocketCallbacks();

        processCommandQueue();

        if (m_currentState)
        {
            StateTransition transition = m_currentState->onTick(m_context);
            if (transition.newState.has_value())
            {
                transitionToState(std::move(transition.newState.value()));
            }
        }

        if (const auto sock = m_context.getSocket())
        {
            sock->tick();
        }
    }

    const char* Reactor::getCurrentStateName() const
    {
        return m_currentState ? m_currentState->getStateName() : "None";
    }

    bool Reactor::isConnected() const
    {
        if (!m_currentState)
        {
            return false;
        }

        return std::strcmp(m_currentState->getStateName(), "Ready") == 0;
    }

    void Reactor::transitionToState(StatePtr newState)
    {
        if (!newState)
        {
            return;
        }

        if (m_currentState)
        {
            m_currentState->onExit(m_context);
        }

        m_currentState = std::move(newState);

        StateTransition enterTransition = m_currentState->onEnter(m_context);

        if (enterTransition.newState.has_value())
        {
            transitionToState(std::move(enterTransition.newState.value()));
        }

        m_socketCallbacksSetup = false;
    }

    void Reactor::processCommandQueue()
    {
        std::deque<Command> commands;
        {
            std::scoped_lock lock(m_commandQueueMutex);
            commands.swap(m_commandQueue);
        }

        for (auto& cmd : commands)
        {
            if (m_currentState)
            {
                StateTransition transition = m_currentState->handleCommand(m_context, cmd);
                if (transition.newState.has_value())
                {
                    transitionToState(std::move(transition.newState.value()));
                }
            }
        }
    }

    void Reactor::setupSocketCallbacks()
    {
        const auto sock = m_context.getSocket();
        if (!sock || m_socketCallbacksSetup)
        {
            return;
        }

        sock->getOnConnectCallback().add(
            [this](const bool wasSuccessful)
            {
                if (!m_currentState)
                {
                    return;
                }

                StateTransition transition = wasSuccessful
                    ? m_currentState->onSocketConnected(m_context)
                    : m_currentState->onSocketDisconnected(m_context);

                if (transition.newState.has_value())
                {
                    transitionToState(std::move(transition.newState.value()));
                }
            });

        sock->getOnDisconnectCallback().add(
            [this]()
            {
                if (!m_currentState)
                {
                    return;
                }

                StateTransition transition = m_currentState->onSocketDisconnected(m_context);
                if (transition.newState.has_value())
                {
                    transitionToState(std::move(transition.newState.value()));
                }

                m_context.invokeCallback(
                    [&ctx = m_context]()
                    {
                        ctx.getOnDisconnect().broadcast(false);
                    });
            });

        sock->getOnDataReceivedCallback().add(
            [this](const uint8_t* data, const uint32_t size)
            {
                if (!m_currentState)
                {
                    return;
                }

                StateTransition transition = m_currentState->onDataReceived(m_context, data, size);
                if (transition.newState.has_value())
                {
                    transitionToState(std::move(transition.newState.value()));
                }
            });

        m_socketCallbacksSetup = true;
    }
} // namespace reactormq::mqtt::client
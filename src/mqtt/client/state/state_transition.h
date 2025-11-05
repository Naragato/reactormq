#pragma once

#include "mqtt/client/command.h"
#include "mqtt/client/context.h"

#include <memory>
#include <optional>

namespace reactormq::mqtt::client
{
    class IState;
    using StatePtr = std::unique_ptr<IState>;
/**
* @brief Result of a state operation, optionally containing a new state to transition to.
 */
struct StateTransition
{
    /**
     * @brief Optional new state. If present, the reactor will transition to this state.
     */
    std::optional<StatePtr> newState;

    /**
     * @brief Static helper to create a transition to a new state.
     * @param state The new state to transition to.
     * @return StateTransition with the new state.
     */
    static StateTransition transitionTo(StatePtr state)
    {
        return StateTransition{std::move(state)};
    }

    /**
     * @brief Static helper to indicate no state transition.
     * @return StateTransition with no new state.
     */
    static StateTransition noTransition()
    {
        return StateTransition{std::nullopt};
    }
};
}
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/client/command.h"

#include <memory>
#include <optional>

namespace reactormq::mqtt::client
{
    class IState;
    using StatePtr = std::unique_ptr<IState>;
    /**
     * @brief Result of a state operation, optionally indicating a new state to transition to.
     */
    struct StateTransition
    {
        std::optional<StatePtr> newState; ///< New state. If present, the reactor will transition to it.

        /**
         * @brief Create a transition to a new state.
         * @param state The new state to transition to.
         * @return Transition with the new state set.
         */
        static StateTransition transitionTo(StatePtr state)
        {
            return StateTransition{ std::move(state) };
        }

        /**
         * @brief Indicate that no state change should occur.
         * @return Transition with no new state.
         */
        static StateTransition noTransition()
        {
            return StateTransition{ std::nullopt };
        }
    };
} // namespace reactormq::mqtt::client
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/client/state/state.h"
#include "mqtt/client/state/state_transition.h"

#include <gtest/gtest.h>

using namespace reactormq::mqtt::client;

namespace
{
    class DummyState final : public IState
    {
    public:
        StateTransition onEnter(Context&) override
        {
            return StateTransition::noTransition();
        }

        void onExit(Context&) override
        {
            // Intentionally empty
        }

        StateTransition handleCommand(Context&, Command&) override
        {
            return StateTransition::noTransition();
        }

        StateTransition onSocketConnected(Context&) override
        {
            return StateTransition::noTransition();
        }

        StateTransition onSocketDisconnected(Context&) override
        {
            return StateTransition::noTransition();
        }

        StateTransition onDataReceived(Context&, const uint8_t*, uint32_t) override
        {
            return StateTransition::noTransition();
        }

        StateTransition onTick(Context&) override
        {
            return StateTransition::noTransition();
        }

        [[nodiscard]] const char* getStateName() const override
        {
            return "Dummy";
        }
        [[nodiscard]] StateId getStateId() const override { return StateId::Ready; }
    };
} // namespace

TEST(StateTransitionTest, DefaultNoTransitionBuilderWorks)
{
    const auto [newState] = StateTransition::noTransition();
    EXPECT_FALSE(newState.has_value());
}

TEST(StateTransitionTest, TransitionToStoresNewState)
{
    const auto [newState] = StateTransition::transitionTo(std::make_unique<DummyState>());
    EXPECT_TRUE(newState.has_value());
}
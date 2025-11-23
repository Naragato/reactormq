#include <gtest/gtest.h>
#include "mqtt/client/command.h"
#include "mqtt/client/reactor.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include <future>

using namespace reactormq::mqtt;
using namespace reactormq::mqtt::client;

TEST(MemoryTest, NoLeaksOnRepeatedConnectDisconnect)
{
    for (int i = 0; i < 1000; ++i)
    {
        ConnectionSettingsBuilder builder;
        builder.setHost("localhost");
        builder.setPort(1883);
        auto settings = builder.build();

        auto reactor = std::make_shared<Reactor>(settings);

        // Connect
        std::promise<Result<void>> pConnect;
        reactor->enqueueCommand(ConnectCommand{true, std::move(pConnect)});

        for (int tick = 0; tick < 100; ++tick)
        {
            reactor->tick();
        }

        // Disconnect
        std::promise<Result<void>> pDisconnect;
        reactor->enqueueCommand(DisconnectCommand{std::move(pDisconnect)});

        reactor.reset();
    }
}

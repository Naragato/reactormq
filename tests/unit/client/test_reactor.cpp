//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "mqtt/client/command.h"
#include "mqtt/client/reactor.h"
#include "mqtt/packets/conn_ack.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include "serialize/bytes.h"
#include "socket/socket.h"

#include <chrono>
#include <future>

using namespace reactormq;
using namespace reactormq::mqtt;
using namespace reactormq::mqtt::client;
using namespace reactormq::socket;

namespace
{
    class FakeSocket final : public Socket
    {
    public:
        explicit FakeSocket(ConnectionSettingsPtr settings)
            : Socket(std::move(settings))
        {
        }

        void connect() override
        {
            isConnectedFlag = true;
        }

        void disconnect() override
        {
            isConnectedFlag = false;
        }

        void close(int32_t /*code*/, const std::string& /*reason*/) override
        {
            isConnectedFlag = false;
        }

        [[nodiscard]] bool isConnected() const override
        {
            return isConnectedFlag;
        }

        void send(const uint8_t* /*data*/, uint32_t /*size*/) override
        {
            /* no-op for tests */
        }

        OnConnectCallback& getOnConnectCallback() override
        {
            return onConnect;
        }

        OnDisconnectCallback& getOnDisconnectCallback() override
        {
            return onDisconnect;
        }

        OnDataReceivedCallback& getOnDataReceivedCallback() override
        {
            return onData;
        }

        void tick() override
        {
            /* no-op */
        }

    private:
        bool isConnectedFlag = false;
        OnConnectCallback onConnect;
        OnDisconnectCallback onDisconnect;
        OnDataReceivedCallback onData;
    };

    ConnectionSettingsPtr makeSettings()
    {
        ConnectionSettingsBuilder b;
        b.setHost("localhost");
        return b.build();
    }
} // namespace

TEST(ReactorTest, InitialStateIsDisconnected)
{
    const Reactor r(makeSettings());
    EXPECT_STREQ(r.getCurrentStateName(), "Disconnected");
    EXPECT_FALSE(r.isConnected());
}

TEST(ReactorTest, EnqueueCommandAddsToQueueAndTickProcesses)
{
    Reactor r(makeSettings());

    std::promise<Result<void>> p;
    auto f = p.get_future();
    DisconnectCommand cmd{ std::move(p) };

    r.enqueueCommand(std::move(cmd));
    r.tick();

    EXPECT_EQ(f.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    const auto res = f.get();
    EXPECT_TRUE(res.isSuccess());
}

TEST(ReactorTest, GetContextReturnsSameContextInstance)
{
    Reactor r(makeSettings());
    auto& a = r.getContext();
    auto& b = r.getContext();
    EXPECT_EQ(&a, &b);
}

TEST(ReactorTest, TransitionToConnectingStateOnConnectCommand)
{
    Reactor r(makeSettings());

    const auto fake = std::make_shared<FakeSocket>(makeSettings());
    r.getContext().setSocket(fake);

    std::promise<Result<void>> p;
    ConnectCommand cmd{ true, std::move(p) };
    r.enqueueCommand(std::move(cmd));

    r.tick();
    EXPECT_STREQ(r.getCurrentStateName(), "Connecting");
}

TEST(ReactorTest, IsConnectedTrueOnlyInReadyState)
{
    Reactor r(makeSettings());

    const auto fake = std::make_shared<FakeSocket>(makeSettings());
    r.getContext().setSocket(fake);

    r.tick();

    std::promise<Result<void>> p;
    ConnectCommand cmd{ true, std::move(p) };
    r.enqueueCommand(std::move(cmd));
    r.tick();
    EXPECT_FALSE(r.isConnected());
    EXPECT_STREQ(r.getCurrentStateName(), "Connecting");

    fake->getOnConnectCallback().broadcast(true);

    using namespace reactormq::mqtt::packets;
    std::vector<std::byte> buf;
    serialize::ByteWriter w(buf);
    const ConnAck<ProtocolVersion::V5> ack(true, ReasonCode::Success, properties::Properties{});
    ack.encode(w);

    fake->getOnDataReceivedCallback().broadcast(reinterpret_cast<const uint8_t*>(buf.data()), static_cast<uint32_t>(buf.size()));

    EXPECT_TRUE(r.isConnected());
    EXPECT_STREQ(r.getCurrentStateName(), "Ready");
}
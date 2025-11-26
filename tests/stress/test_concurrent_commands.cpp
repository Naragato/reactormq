//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "fixtures/docker_container.h"
#include "fixtures/port_utils.h"
#include "mqtt/client/command.h"
#include "mqtt/client/reactor.h"
#include "reactormq/mqtt/connection_protocol.h"
#include "reactormq/mqtt/connection_settings.h"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace reactormq::mqtt;
using namespace reactormq::mqtt::client;
using reactormq::test::DockerContainer;

class ReactorStressTest : public testing::Test
{
protected:
    void SetUp() override
    {
        if (!DockerContainer::isDockerAvailable())
        {
            GTEST_SKIP() << "Docker not available.";
        }

        auto portOpt = reactormq::test::findAvailablePort(5000, 10000);
        if (!portOpt.has_value())
        {
            GTEST_SKIP() << "No available TCP port found.";
        }
        m_port = *portOpt;

        std::string runArgs = "-e DOCKER_VERNEMQ_ACCEPT_EULA=yes -e DOCKER_VERNEMQ_ALLOW_ANONYMOUS=on";
        m_container = std::make_unique<DockerContainer>("reactormq_stress_test", "vernemq/vernemq", m_port, 1883, runArgs);

        if (!m_container->startAsync(std::chrono::seconds(60)).get())
        {
            GTEST_SKIP() << "Failed to start docker broker container.";
        }
    }

    void TearDown() override
    {
        if (m_container)
        {
            m_container->stop();
        }
    }

    uint16_t getPort() const
    {
        return m_port;
    }

private:
    uint16_t m_port = 0;
    std::unique_ptr<DockerContainer> m_container;
};

TEST_F(ReactorStressTest, ConcurrentCommandEnqueue)
{
    auto settings = std::make_shared<ConnectionSettings>("127.0.0.1", getPort(), ConnectionProtocol::Tcp, nullptr);
    Reactor reactor(settings);

    constexpr size_t kThreadCount = 16;
    constexpr size_t kCommandsPerThread = 1000;

    std::vector<std::jthread> threads;
    std::atomic<size_t> enqueuedCount{ 0 };

    for (size_t i = 0; i < kThreadCount; ++i)
    {
        threads.emplace_back(
            [&reactor, &enqueuedCount]()
            {
                for (size_t j = 0; j < kCommandsPerThread; ++j)
                {
                    ConnectCommand cmd;
                    cmd.cleanSession = true;
                    reactor.enqueueCommand(std::move(cmd));
                    ++enqueuedCount;
                    std::this_thread::yield();
                }
            });
    }

    std::jthread processorThread(
        [&reactor, &enqueuedCount]()
        {
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < std::chrono::seconds(10))
            {
                reactor.tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (enqueuedCount.load() == kThreadCount * kCommandsPerThread
                    && std::chrono::steady_clock::now() - start > std::chrono::seconds(2))
                {
                    break;
                }
            }
        });

    for (auto& thread : threads)
    {
        thread.join();
    }

    processorThread.join();

    EXPECT_EQ(enqueuedCount.load(), kThreadCount * kCommandsPerThread);
}

TEST_F(ReactorStressTest, DeadlockDetection)
{
    auto settings = std::make_shared<ConnectionSettings>("127.0.0.1", getPort(), ConnectionProtocol::Tcp, nullptr);
    Reactor reactor(settings);

    std::jthread t1(
        [&reactor]()
        {
            for (int i = 0; i < 1000; ++i)
            {
                ConnectCommand cmd;
                cmd.cleanSession = true;
                reactor.enqueueCommand(std::move(cmd));
            }
        });

    std::jthread t2(
        [&reactor]()
        {
            for (int i = 0; i < 1000; ++i)
            {
                reactor.tick();
                std::this_thread::yield();
            }
        });

    t1.join();
    t2.join();

    SUCCEED();
}
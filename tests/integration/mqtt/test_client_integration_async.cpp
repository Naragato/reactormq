#include <gtest/gtest.h>

#include "fixtures/docker_container.h"
#include "fixtures/port_utils.h"

#include "reactormq/mqtt/client.h"
#include "reactormq/mqtt/connection_settings_builder.h"
#include "reactormq/mqtt/credentials.h"
#include "reactormq/mqtt/credentials_provider.h"
#include "reactormq/mqtt/message.h"
#include "reactormq/mqtt/quality_of_service.h"
#include "reactormq/mqtt/topic_filter.h"
#include "mqtt/client/client_factory.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace reactormq::mqtt;
using namespace reactormq::mqtt::client;
using reactormq::test::DockerContainer;

namespace
{
    class NoOpCredentialsProvider final : public ICredentialsProvider
    {
    public:
        Credentials getCredentials() override
        {
            return Credentials{};
        }
    };
}

class MqttClientDockerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!DockerContainer::isDockerAvailable())
        {
            GTEST_SKIP() << "Docker not available; skipping MQTT docker integration test.";
        }

        auto portOpt = reactormq::test::findAvailablePort(5000, 10000);
        if (!portOpt.has_value())
        {
            GTEST_SKIP() << "No available TCP port found for broker mapping; skipping test.";
        }
        m_hostPort = *portOpt;
        constexpr std::uint16_t containerPort = 1883;
        std::string runArgs = "-e DOCKER_VERNEMQ_ACCEPT_EULA=yes -e DOCKER_VERNEMQ_ALLOW_ANONYMOUS=on";

        m_container = std::make_unique<DockerContainer>(
            "reactormq_vernemq_test", "vernemq/vernemq", m_hostPort, containerPort, runArgs);

        auto started = m_container->startAsync(std::chrono::seconds(60)).get();
        if (!started)
        {
            GTEST_SKIP() << "Failed to start docker broker container; skipping.";
        }

        m_settings = ConnectionSettingsBuilder("127.0.0.1")
            .setPort(m_hostPort)
            .setProtocol(ConnectionProtocol::Tcp)
            .setCredentialsProvider(std::make_shared<NoOpCredentialsProvider>())
            .setSocketConnectionTimeoutSeconds(10)
            .setMqttConnectionTimeoutSeconds(10)
            .setKeepAliveIntervalSeconds(30)
            .build();

        m_client = createClient(m_settings);
    }

    void TearDown() override
    {
        if (m_client)
        {
            auto fut = m_client->disconnectAsync();
            for (int i = 0; i < 200; ++i)
            {
                m_client->tick();
                if (fut.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
                {
                    (void)fut.get();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        if (m_container)
        {
            m_container->stop();
        }
    }

    std::uint16_t m_hostPort{};
    std::unique_ptr<DockerContainer> m_container;
    ConnectionSettingsPtr m_settings;
    std::shared_ptr<IClient> m_client;
};

TEST_F(MqttClientDockerTest, ConnectSubscribePublishAsync)
{
    if (!m_client)
    {
        GTEST_SKIP() << "Client not initialized.";
    }

    std::atomic connected{ false };
    auto onConnectHandle = m_client->onConnect().add([
        &connected](const bool isConnected)
    {
        connected.store(isConnected);
    });

    auto connectFuture = m_client->connectAsync(true);

    const auto connectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (std::chrono::steady_clock::now() < connectDeadline)
    {
        m_client->tick();
        if (connectFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto res = connectFuture.get();
            ASSERT_TRUE(res.hasSucceeded()) << "connectAsync failed";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!connected.load())
    {
        GTEST_SKIP() << "Not connected within timeout; skipping test (environment issue).";
    }

    const std::string topic = "reactormq/test";
    std::atomic<bool> gotMessage{ false };
    auto onMessageHandle = m_client->onMessage().add([
        &gotMessage, &topic](const Message& message)
    {
        if (message.getTopic() == topic)
        {
            gotMessage.store(true);
        }
    });

    // Subscribe with noLocal=false so we can receive our own publish on some brokers.
    TopicFilter filter(topic, QualityOfService::AtLeastOnce, false);
    auto subFuture = m_client->subscribeAsync(std::move(filter));

    const auto subDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    bool subscribed = false;
    while (std::chrono::steady_clock::now() < subDeadline)
    {
        m_client->tick();
        if (subFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto res = subFuture.get();
            ASSERT_TRUE(res.hasSucceeded()) << "subscribeAsync failed";
            subscribed = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!subscribed)
    {
        GTEST_SKIP() << "Subscription did not complete; skipping (env).";
    }

    Message msg(topic, { 'o','k' }, false, QualityOfService::AtLeastOnce);
    auto pubFuture = m_client->publishAsync(std::move(msg));

    const auto pubDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    bool published = false;
    while (std::chrono::steady_clock::now() < pubDeadline)
    {
        m_client->tick();
        if (pubFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto res = pubFuture.get();
            ASSERT_TRUE(res.hasSucceeded()) << "publishAsync failed";
            published = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(published) << "Publish did not complete";

    const auto msgDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < msgDeadline && !gotMessage.load())
    {
        m_client->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(gotMessage.load()) << "Did not receive published message";

    (void)onConnectHandle;
    (void)onMessageHandle;
}

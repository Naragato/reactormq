//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "reactormq/mqtt/connection_settings.h"
#include "reactormq/mqtt/credentials_provider.h"

#include <memory>
#include <string>

using namespace reactormq::mqtt;

namespace
{
    class StaticCredentialsProvider final : public ICredentialsProvider
    {
    public:
        explicit StaticCredentialsProvider(std::string user, std::string pass)
            : m_creds(std::move(user), std::move(pass))
        {
        }

        Credentials getCredentials() override
        {
            return m_creds;
        }

    private:
        Credentials m_creds;
    };
} // namespace

TEST(MqttTypes_ConnectionSettings, DefaultsAndGetters)
{
    auto provider = std::make_shared<StaticCredentialsProvider>("bob", "pw");

    ConnectionSettings s{ "example.com", static_cast<uint16_t>(1883), ConnectionProtocol::Tcp, provider };

    EXPECT_EQ(s.getHost(), "example.com");
    EXPECT_EQ(s.getPort(), 1883);
    EXPECT_EQ(s.getProtocol(), ConnectionProtocol::Tcp);
    EXPECT_EQ(s.getCredentialsProvider(), provider);
    EXPECT_TRUE(s.getPath().empty());
    EXPECT_TRUE(s.getClientId().empty());

    EXPECT_EQ(s.getMaxPacketSize(), 1u * 1024u * 1024u);
    EXPECT_EQ(s.getMaxBufferSize(), 64u * 1024u * 1024u);
    EXPECT_EQ(s.getPacketRetryIntervalSeconds(), 5);
    EXPECT_NEAR(s.getPacketRetryBackoffMultiplier(), 1.5, 1e-9);
    EXPECT_EQ(s.getMaxPacketRetryIntervalSeconds(), 60);
    EXPECT_EQ(s.getSocketConnectionTimeoutSeconds(), 30);
    EXPECT_EQ(s.getKeepAliveIntervalSeconds(), 60);
    EXPECT_EQ(s.getMqttConnectionTimeoutSeconds(), 30);
    EXPECT_EQ(s.getInitialRetryIntervalSeconds(), 5);
    EXPECT_EQ(s.getMaxConnectionRetries(), 5);
    EXPECT_EQ(s.getMaxPacketRetries(), 3);
    EXPECT_TRUE(s.shouldVerifyServerCertificate());
    EXPECT_EQ(s.getSessionExpiryInterval(), 0u);
}
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "reactormq/mqtt/credentials_provider.h"

#include <string>
#include <vector>

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

TEST(MqttTypes_CredentialsProvider, ProviderBasicsAndEnhancedAuthDefaults)
{
    StaticCredentialsProvider provider{ "alice", "secret" };

    const auto creds = provider.getCredentials();
    EXPECT_EQ(creds.username, "alice");
    EXPECT_EQ(creds.password, "secret");

    EXPECT_TRUE(provider.getAuthMethod().empty());
    EXPECT_TRUE(provider.getInitialAuthData().empty());

    const std::vector<uint8_t> challenge{ 1, 2, 3 };
    EXPECT_TRUE(provider.onAuthChallenge(challenge).empty());
}
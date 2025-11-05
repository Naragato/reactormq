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
}

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
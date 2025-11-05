#include <gtest/gtest.h>

#include "reactormq/mqtt/credentials.h"

using namespace reactormq::mqtt;

TEST(MqttTypes_Credentials, CredentialsConstruction)
{
    const Credentials c1;
    EXPECT_TRUE(c1.username.empty());
    EXPECT_TRUE(c1.password.empty());

    const Credentials c2{ "user", "pass" };
    EXPECT_EQ(c2.username, "user");
    EXPECT_EQ(c2.password, "pass");
}
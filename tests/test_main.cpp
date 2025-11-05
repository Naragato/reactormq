#include "fixtures/test_utils.h"

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new reactormq::tests::SocketTestEnvironment());
    return RUN_ALL_TESTS();
}
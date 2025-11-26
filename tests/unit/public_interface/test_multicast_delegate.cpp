//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <gtest/gtest.h>

#include "reactormq/mqtt/delegates.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace reactormq::mqtt;

namespace
{
    class TestListener
    {
    public:
        void onEvent(const int value)
        {
            m_lastValue = value;
            ++m_callCount;
        }

        int onEventWithReturn(const int value)
        {
            m_lastValue = value;
            ++m_callCount;
            return value * 2;
        }

        [[nodiscard]] int getLastValue() const
        {
            return m_lastValue;
        }

        [[nodiscard]] int getCallCount() const
        {
            return m_callCount;
        }

    private:
        int m_lastValue = 0;
        int m_callCount = 0;
    };
} // namespace

TEST(Delegates_MulticastDelegate, VoidReturnType_BasicAddAndBroadcast)
{
    MulticastDelegate<void(int)> delegate;

    int callCount = 0;
    int lastValue = 0;

    const auto conn = delegate.add(
        [&callCount, &lastValue](const int value)
        {
            ++callCount;
            lastValue = value;
        });

    EXPECT_TRUE(conn.isValid());
    EXPECT_EQ(delegate.getSize(), 1u);

    delegate.broadcast(42);

    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastValue, 42);
}

TEST(Delegates_MulticastDelegate, VoidReturnType_MultipleCallbacks)
{
    MulticastDelegate<void(int)> delegate;

    int sum = 0;

    auto conn1 = delegate.add(
        [&sum](const int value)
        {
            sum += value;
        });
    auto conn2 = delegate.add(
        [&sum](const int value)
        {
            sum += value * 2;
        });
    auto conn3 = delegate.add(
        [&sum](const int value)
        {
            sum += value * 3;
        });

    EXPECT_EQ(delegate.getSize(), 3u);

    delegate.broadcast(10);

    EXPECT_EQ(sum, 60);
}

TEST(Delegates_MulticastDelegate, NonVoidReturnType_CollectsResults)
{
    MulticastDelegate<int(int)> delegate;

    auto handle1 = delegate.add(
        [](const int x)
        {
            return x * 2;
        });
    auto handle2 = delegate.add(
        [](const int x)
        {
            return x * 3;
        });
    auto handle3 = delegate.add(
        [](const int x)
        {
            return x * 4;
        });

    EXPECT_EQ(delegate.getSize(), 3u);

    auto results = delegate.broadcast(5);

    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0], 10);
    EXPECT_EQ(results[1], 15);
    EXPECT_EQ(results[2], 20);
}

TEST(Delegates_MulticastDelegate, NonVoidReturnType_EmptyDelegate)
{
    MulticastDelegate<int(int)> delegate;

    const auto results = delegate.broadcast(42);

    EXPECT_TRUE(results.empty());
}

TEST(Delegates_MulticastDelegate, MemberFunctionBinding)
{
    MulticastDelegate<void(int)> delegate;

    const auto listener = std::make_shared<TestListener>();
    const auto conn = delegate.addMember(listener, &TestListener::onEvent);

    EXPECT_TRUE(conn.isValid());
    EXPECT_EQ(delegate.getSize(), 1u);

    delegate.broadcast(100);

    EXPECT_EQ(listener->getLastValue(), 100);
    EXPECT_EQ(listener->getCallCount(), 1);

    delegate.broadcast(200);

    EXPECT_EQ(listener->getLastValue(), 200);
    EXPECT_EQ(listener->getCallCount(), 2);
}

TEST(Delegates_MulticastDelegate, MemberFunctionBinding_WithReturn)
{
    MulticastDelegate<int(int)> delegate;

    const auto listener = std::make_shared<TestListener>();
    auto handle = delegate.addMember(listener, &TestListener::onEventWithReturn);

    const auto results = delegate.broadcast(7);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0], 14);
    EXPECT_EQ(listener->getLastValue(), 7);
    EXPECT_EQ(listener->getCallCount(), 1);
}

TEST(Delegates_MulticastDelegate, AddCallable_WithSharedPtr)
{
    MulticastDelegate<std::string(const std::string&)> delegate;

    const auto callable = std::make_shared<std::function<std::string(const std::string&)>>(
        [](const std::string& input)
        {
            return input + "_processed";
        });

    const auto conn = delegate.addCallable(callable);

    EXPECT_TRUE(conn.isValid());

    const auto results = delegate.broadcast("test");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0], "test_processed");
}

TEST(Delegates_MulticastDelegate, ConnectionDisconnect)
{
    MulticastDelegate<void(int)> delegate;

    int callCount = 0;

    auto conn = delegate.add(
        [&callCount](int)
        {
            ++callCount;
        });

    EXPECT_EQ(delegate.getSize(), 1u);
    EXPECT_TRUE(conn.isValid());

    delegate.broadcast(1);
    EXPECT_EQ(callCount, 1);

    conn.disconnect();

    EXPECT_FALSE(conn.isValid());
    EXPECT_EQ(delegate.getSize(), 0u);

    delegate.broadcast(2);
    EXPECT_EQ(callCount, 1); // Should not increase
}

TEST(Delegates_MulticastDelegate, ConnectionDisconnect_Multiple)
{
    MulticastDelegate<void()> delegate;

    int count1 = 0;
    int count2 = 0;
    int count3 = 0;

    auto conn1 = delegate.add(
        [&count1]
        {
            ++count1;
        });
    auto conn2 = delegate.add(
        [&count2]
        {
            ++count2;
        });
    auto conn3 = delegate.add(
        [&count3]
        {
            ++count3;
        });

    EXPECT_EQ(delegate.getSize(), 3u);

    delegate.broadcast();
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 1);

    conn2.disconnect();

    EXPECT_EQ(delegate.getSize(), 2u);

    delegate.broadcast();
    EXPECT_EQ(count1, 2);
    EXPECT_EQ(count2, 1); // Unchanged
    EXPECT_EQ(count3, 2);
}

TEST(Delegates_MulticastDelegate, ManualRemove)
{
    MulticastDelegate<void(int)> delegate;

    int callCount = 0;

    auto conn = delegate.add(
        [&callCount](int)
        {
            ++callCount;
        });

    EXPECT_EQ(delegate.getSize(), 1u);

    delegate.broadcast(1);
    EXPECT_EQ(callCount, 1);

    conn.disconnect();

    delegate.broadcast(2);
    EXPECT_EQ(callCount, 1); // Should not increase
}

TEST(Delegates_MulticastDelegate, ClearAll)
{
    MulticastDelegate<void(int)> delegate;

    int count1 = 0;
    int count2 = 0;
    int count3 = 0;

    auto handle1 = delegate.add(
        [&count1](int)
        {
            ++count1;
        });
    auto handle2 = delegate.add(
        [&count2](int)
        {
            ++count2;
        });
    auto handle3 = delegate.add(
        [&count3](int)
        {
            ++count3;
        });

    EXPECT_EQ(delegate.getSize(), 3u);

    delegate.broadcast(1);
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 1);

    delegate.clear();

    EXPECT_EQ(delegate.getSize(), 0u);

    delegate.broadcast(2);
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 1);
}

TEST(Delegates_MulticastDelegate, AutoCleanup_ExpiredWeakReferences)
{
    MulticastDelegate<void(int)> delegate;

    int count = 0;

    {
        const auto listener = std::make_shared<TestListener>();
        auto memberHandle = delegate.addMember(listener, &TestListener::onEvent);
        EXPECT_EQ(delegate.getSize(), 1u);

        auto lambdaHandle = delegate.add(
            [&count](int)
            {
                ++count;
            });
        EXPECT_EQ(delegate.getSize(), 2u);
    } // listener goes out of scope, weak_ptr should expire

    delegate.broadcast(1);

    EXPECT_EQ(delegate.getSize(), 1u);
    EXPECT_EQ(count, 1);
}

TEST(Delegates_MulticastDelegate, AutoCleanup_MemberFunctionExpires)
{
    MulticastDelegate<int(int)> delegate;

    {
        const auto listener = std::make_shared<TestListener>();
        delegate.addMember(listener, &TestListener::onEventWithReturn);

        const auto results = delegate.broadcast(5);
        ASSERT_EQ(results.size(), 1u);
        EXPECT_EQ(results[0], 10);
    } // listener destroyed

    const auto results = delegate.broadcast(10);
    EXPECT_TRUE(results.empty());
}

TEST(Delegates_MulticastDelegate, ConnectionInvalidation_WhenDelegateDestroyed)
{
    DelegateHandle handle;

    {
        MulticastDelegate<void()> delegate;
        handle = delegate.add(
            []
            {
                // Intentionally Empty
            });

        EXPECT_TRUE(handle.isValid());
    } // delegate destroyed

    EXPECT_FALSE(handle.isValid());

    handle.disconnect();
    EXPECT_FALSE(handle.isValid());
}

TEST(Delegates_MulticastDelegate, GetSize_TracksCorrectly)
{
    MulticastDelegate<void()> delegate;

    EXPECT_EQ(delegate.getSize(), 0u);

    auto conn1 = delegate.add(
        []
        {
            // Intentionally empty
        });
    EXPECT_EQ(delegate.getSize(), 1u);

    auto conn2 = delegate.add(
        []
        {
            // Intentionally empty
        });
    EXPECT_EQ(delegate.getSize(), 2u);

    auto conn3 = delegate.add(
        []
        {
            // Intentionally empty
        });
    EXPECT_EQ(delegate.getSize(), 3u);

    conn2.disconnect();
    EXPECT_EQ(delegate.getSize(), 2u);

    conn1.disconnect();
    EXPECT_EQ(delegate.getSize(), 1u);

    conn3.disconnect();
    EXPECT_EQ(delegate.getSize(), 0u);
}

TEST(Delegates_MulticastDelegate, ThreadSafety_ConcurrentAdds)
{
    MulticastDelegate<void(int)> delegate;
    std::atomic totalCalls{ 0 };

    constexpr int kNumThreads = 10;
    constexpr int kCallbacksPerThread = 100;

    std::vector<std::jthread> threads;

    threads.reserve(kNumThreads);
    for (int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back(
            [&delegate, &totalCalls]
            {
                std::vector<DelegateHandle> handles;
                handles.reserve(kCallbacksPerThread);
                for (int i = 0; i < kCallbacksPerThread; ++i)
                {
                    handles.push_back(delegate.add(
                        [&totalCalls](int)
                        {
                            totalCalls.fetch_add(1, std::memory_order_relaxed);
                        }));
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(delegate.getSize(), static_cast<size_t>(kNumThreads * kCallbacksPerThread));

    delegate.broadcast(1);

    EXPECT_EQ(totalCalls.load(), kNumThreads * kCallbacksPerThread);
}

TEST(Delegates_MulticastDelegate, ThreadSafety_ConcurrentBroadcasts)
{
    MulticastDelegate<void(int)> delegate;
    std::atomic totalCalls{ 0 };

    constexpr int kNumCallbacks = 10;
    constexpr int kNumBroadcastThreads = 10;
    constexpr int kBroadcastsPerThread = 100;

    std::vector<DelegateHandle> handles;
    handles.reserve(kNumCallbacks);
    for (int i = 0; i < kNumCallbacks; ++i)
    {
        handles.push_back(delegate.add(
            [&totalCalls](int)
            {
                totalCalls.fetch_add(1, std::memory_order_relaxed);
            }));
    }

    std::vector<std::jthread> threads;

    threads.reserve(kNumBroadcastThreads);
    for (int t = 0; t < kNumBroadcastThreads; ++t)
    {
        threads.emplace_back(
            [&delegate]
            {
                for (int i = 0; i < kBroadcastsPerThread; ++i)
                {
                    delegate.broadcast(i);
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(totalCalls.load(), kNumCallbacks * kNumBroadcastThreads * kBroadcastsPerThread);
}

TEST(Delegates_MulticastDelegate, ThreadSafety_ConcurrentAddRemoveBroadcast)
{
    MulticastDelegate<void(int)> delegate;
    std::atomic totalCalls{ 0 };
    std::atomic stopFlag{ false };

    std::jthread adder(
        [&stopFlag, &delegate, &totalCalls]
        {
            std::vector<DelegateHandle> connections;
            while (!stopFlag.load(std::memory_order_relaxed))
            {
                connections.push_back(delegate.add(
                    [&totalCalls](int)
                    {
                        totalCalls.fetch_add(1, std::memory_order_relaxed);
                    }));
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });

    std::jthread broadcaster(
        [&delegate]
        {
            for (int i = 0; i < 100; ++i)
            {
                delegate.broadcast(i);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });

    std::jthread clearer(
        [&delegate]
        {
            for (int i = 0; i < 10; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                delegate.clear();
            }
        });

    broadcaster.join();
    clearer.join();
    stopFlag.store(true, std::memory_order_relaxed);
    adder.join();

    SUCCEED();
}

TEST(Delegates_MulticastDelegate, OrderPreserved)
{
    MulticastDelegate<void(int)> delegate;

    std::vector<int> order;

    auto handle1 = delegate.add(
        [&order](const int val)
        {
            order.push_back(val * 1);
        });
    auto handle2 = delegate.add(
        [&order](const int val)
        {
            order.push_back(val * 2);
        });
    auto handle3 = delegate.add(
        [&order](const int val)
        {
            order.push_back(val * 3);
        });

    delegate.broadcast(1);

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST(Delegates_MulticastDelegate, NonVoidReturnType_OrderPreserved)
{
    MulticastDelegate<int(int)> delegate;

    auto handle1 = delegate.add(
        [](const int x)
        {
            return x + 1;
        });
    auto handle2 = delegate.add(
        [](const int x)
        {
            return x + 2;
        });
    auto handle3 = delegate.add(
        [](const int x)
        {
            return x + 3;
        });

    auto results = delegate.broadcast(10);

    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0], 11);
    EXPECT_EQ(results[1], 12);
    EXPECT_EQ(results[2], 13);
}

TEST(Delegates_Connection, DefaultConstructor)
{
    DelegateHandle handle;

    EXPECT_FALSE(handle.isValid());

    handle.disconnect();
    EXPECT_FALSE(handle.isValid());
}

TEST(Delegates_Connection, MultipleDisconnects)
{
    MulticastDelegate<void()> delegate;

    auto conn = delegate.add(
        []
        {
            // Intentionally empty
        });

    EXPECT_TRUE(conn.isValid());

    conn.disconnect();
    EXPECT_FALSE(conn.isValid());

    conn.disconnect();
    EXPECT_FALSE(conn.isValid());

    conn.disconnect();
    EXPECT_FALSE(conn.isValid());
}
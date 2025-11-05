#include "reactormq/mqtt/connection_protocol.h"
#include "reactormq/mqtt/connection_settings.h"
#include "socket/socket_delegate_mixin.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>

using reactormq::mqtt::ConnectionSettings;

namespace
{
    std::vector<uint8_t> encodeRemainingLength(uint32_t value)
    {
        std::vector<uint8_t> out;
        do
        {
            auto encodedByte = static_cast<uint8_t>(value % 128u);
            value /= 128u;
            if (value > 0)
            {
                encodedByte |= 128u;
            }
            out.push_back(encodedByte);
        } while (value > 0);
        return out;
    }

    std::vector<uint8_t> makePacket(const uint32_t payloadSize)
    {
        std::vector<uint8_t> data;
        data.reserve(1 + 4 + payloadSize);
        data.push_back(0x30);
        auto rl = encodeRemainingLength(payloadSize);
        data.insert(data.end(), rl.begin(), rl.end());
        data.resize(data.size() + payloadSize, 0xAB);
        return data;
    }

    class TestSocketBase final : public reactormq::socket::SocketDelegateMixin<TestSocketBase>
    {
    public:
        explicit TestSocketBase(const std::shared_ptr<ConnectionSettings>& settings)
            : SocketDelegateMixin(settings)
        {
            m_dataHandle = SocketDelegateMixin::getOnDataReceivedCallback().add(
                [this](const uint8_t* d, const uint32_t s)
                {
                    packets.emplace_back(d, d + s);
                });
        }

        void connect() override
        {
        }

        void disconnect() override
        {
        }

        void close(int32_t code, const std::string& reason) override
        {
        }

        [[nodiscard]] bool isConnected() const override
        {
            return true;
        }

        void send(const uint8_t* /*data*/, uint32_t /*size*/) override
        {
        }

        void tick() override
        {
        }

        void feed(const std::vector<uint8_t>& bytes)
        {
            m_dataBuffer.insert(m_dataBuffer.end(), bytes.begin(), bytes.end());
        }

        void feed(const uint8_t* bytes, const size_t len)
        {
            m_dataBuffer.insert(m_dataBuffer.end(), bytes, bytes + len);
        }

        bool parse()
        {
            return readPacketsFromBuffer();
        }

        [[nodiscard]] size_t bufferSize() const
        {
            return m_dataBuffer.size();
        }

        [[nodiscard]] size_t readOffset() const
        {
            return m_dataBufferReadOffset;
        }

        std::vector<std::vector<uint8_t>> packets;
        reactormq::mqtt::DelegateHandle m_dataHandle;
    };

    std::shared_ptr<ConnectionSettings> makeSettings(uint32_t maxPacketSize = 1 * 1024 * 1024)
    {
        constexpr uint32_t maxBufferSize = 64 * 1024 * 1024;
        using namespace reactormq::mqtt;
        return std::make_shared<ConnectionSettings>(
            "localhost", 1883, ConnectionProtocol::Tcp, nullptr, "/", "test-client", maxPacketSize, maxBufferSize);
    }

    TEST(SocketBaseParser, SingleCompletePacket)
    {
        const auto settings = makeSettings();
        TestSocketBase sock(settings);

        const auto pkt = makePacket(10);
        sock.feed(pkt);
        ASSERT_TRUE(sock.parse());

        ASSERT_EQ(sock.packets.size(), 1u);
        EXPECT_EQ(sock.packets[0], pkt);
    }

    TEST(SocketBaseParser, SplitAcrossReads)
    {
        const auto settings = makeSettings();
        TestSocketBase sock(settings);

        const auto pkt = makePacket(256);

        const size_t half = pkt.size() / 2;
        sock.feed(pkt.data(), half);
        ASSERT_TRUE(sock.parse());
        EXPECT_TRUE(sock.packets.empty());

        sock.feed(pkt.data() + half, pkt.size() - half);
        ASSERT_TRUE(sock.parse());
        ASSERT_EQ(sock.packets.size(), 1u);
        EXPECT_EQ(sock.packets[0], pkt);
    }

    TEST(SocketBaseParser, BackToBackPackets)
    {
        const auto settings = makeSettings();
        TestSocketBase sock(settings);

        auto a = makePacket(5);
        auto b = makePacket(7);
        std::vector<uint8_t> both;
        both.reserve(a.size() + b.size());
        both.insert(both.end(), a.begin(), a.end());
        both.insert(both.end(), b.begin(), b.end());

        sock.feed(both);
        ASSERT_TRUE(sock.parse());

        ASSERT_EQ(sock.packets.size(), 2u);
        EXPECT_EQ(sock.packets[0], a);
        EXPECT_EQ(sock.packets[1], b);
    }

    TEST(SocketBaseParser, TooLargePacketFails)
    {
        const auto settings = makeSettings(/*maxPacketSize*/ 16);
        TestSocketBase sock(settings);

        const auto pkt = makePacket(128);
        sock.feed(pkt);

        ASSERT_FALSE(sock.parse());
        EXPECT_TRUE(sock.packets.empty());
    }

    TEST(SocketBaseParser, BufferCompactionThreshold)
    {
        const auto settings = makeSettings();
        TestSocketBase sock(settings);

        constexpr uint32_t bigPayload = 70 * 1024;
        constexpr int count = 4;

        std::vector<uint8_t> stream;
        for (int i = 0; i < count; ++i)
        {
            auto pkt = makePacket(bigPayload);
            stream.insert(stream.end(), pkt.begin(), pkt.end());
        }

        std::vector<uint8_t> partial;
        partial.push_back(0x30);
        const auto rl = encodeRemainingLength(bigPayload);
        partial.push_back(rl[0]);

        stream.insert(stream.end(), partial.begin(), partial.end());

        sock.feed(stream);

        ASSERT_TRUE(sock.parse());

        ASSERT_EQ(sock.packets.size(), static_cast<size_t>(count));

        EXPECT_EQ(sock.readOffset(), 0u);
        EXPECT_EQ(sock.bufferSize(), partial.size());

        std::vector<uint8_t> remainder;
        for (size_t i = 1; i < rl.size(); ++i)
            remainder.push_back(rl[i]);
        remainder.resize(remainder.size() + bigPayload, 0xAB);

        sock.feed(remainder);

        ASSERT_TRUE(sock.parse());
        ASSERT_EQ(sock.packets.size(), static_cast<size_t>(count + 1));
    }
} // namespace
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/properties/property.h"

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

using namespace reactormq::mqtt::packets::properties;
using namespace reactormq::serialize;

using enum PropertyIdentifier;

ByteReader makeReader(const std::vector<std::byte>& data)
{
    return { data.data(), data.size() };
}

Property makeProperty(const PropertyIdentifier id)
{
    constexpr uint8_t value = 42;

    switch (id)
    {
    case PayloadFormatIndicator:
        return Property::create<PayloadFormatIndicator, uint8_t>(value);
    case RequestProblemInformation:
        return Property::create<RequestProblemInformation, uint8_t>(value);
    case RequestResponseInformation:
        return Property::create<RequestResponseInformation, uint8_t>(value);
    case MaximumQoS:
        return Property::create<MaximumQoS, uint8_t>(value);
    case RetainAvailable:
        return Property::create<RetainAvailable, uint8_t>(value);
    case WildcardSubscriptionAvailable:
        return Property::create<WildcardSubscriptionAvailable, uint8_t>(value);
    case SubscriptionIdentifierAvailable:
        return Property::create<SubscriptionIdentifierAvailable, uint8_t>(value);
    default:
        return Property::create<SharedSubscriptionAvailable, uint8_t>(value);
    }
}

TEST(Property, CreateAndEncode_Uint8_PayloadFormatIndicator)
{
    const auto prop = Property::create<PayloadFormatIndicator, uint8_t>(1);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::PayloadFormatIndicator);
    EXPECT_EQ(prop.getLength(), 2u);

    uint8_t value = 0;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value, 1);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    prop.encode(writer);

    EXPECT_EQ(buffer.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 1);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 1);
}

TEST(Property, CreateAndEncode_Uint16_ServerKeepAlive)
{
    const auto prop = Property::create<ServerKeepAlive, uint16_t>(300);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::ServerKeepAlive);
    EXPECT_EQ(prop.getLength(), 3u);

    uint16_t value = 0;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value, 300);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    prop.encode(writer);

    EXPECT_EQ(buffer.size(), 3u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 19);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x2C);
}

// Test uint32 property - MessageExpiryInterval
TEST(Property, CreateAndEncode_Uint32_MessageExpiryInterval)
{
    const auto prop = Property::create<MessageExpiryInterval, uint32_t>(3600);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::MessageExpiryInterval);
    EXPECT_EQ(prop.getLength(), 5u);

    uint32_t value = 0;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value, 3600);
}

// Test SubscriptionIdentifier with VBI encoding
TEST(Property, CreateAndEncode_VBI_SubscriptionIdentifier)
{
    const auto prop = Property::create<SubscriptionIdentifier, uint32_t>(128);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::SubscriptionIdentifier);
    EXPECT_EQ(prop.getLength(), 3u);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    prop.encode(writer);

    EXPECT_EQ(buffer.size(), 3u);
    EXPECT_EQ(static_cast<uint8_t>(buffer[0]), 11);
    EXPECT_EQ(static_cast<uint8_t>(buffer[1]), 0x80);
    EXPECT_EQ(static_cast<uint8_t>(buffer[2]), 0x01);
}

// Test string property - ContentType
TEST(Property, CreateAndEncode_String_ContentType)
{
    const auto prop = Property::create<ContentType, std::string>("application/json");

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::ContentType);
    EXPECT_EQ(prop.getLength(), 19u);

    std::string value;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value, "application/json");
}

// Test binary data property - CorrelationData
TEST(Property, CreateAndEncode_Binary_CorrelationData)
{
    const std::vector<uint8_t> data = { 0x01, 0x02, 0x03, 0x04 };
    const auto prop = Property::create<CorrelationData, std::vector<uint8_t>>(data);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::CorrelationData);
    EXPECT_EQ(prop.getLength(), 7u);

    std::vector<uint8_t> value;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value, data);
}

// Test UserProperty (pair of strings)
TEST(Property, CreateAndEncode_UserProperty)
{
    const auto prop = Property::create<UserProperty, std::pair<std::string, std::string>>(std::make_pair("key", "value"));

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::UserProperty);
    EXPECT_EQ(prop.getLength(), 13u);

    std::pair<std::string, std::string> value;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value.first, "key");
    EXPECT_EQ(value.second, "value");
}

// Test decode uint8
TEST(Property, Decode_Uint8)
{
    const std::vector buffer = { std::byte{ 1 }, std::byte{ 1 } };

    auto reader = makeReader(buffer);
    const Property prop(reader);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::PayloadFormatIndicator);

    uint8_t value = 0;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value, 1);
}

// Test decode uint16
TEST(Property, Decode_Uint16)
{
    const std::vector buffer = { std::byte{ 19 }, std::byte{ 0x01 }, std::byte{ 0x2C } };

    auto reader = makeReader(buffer);
    const Property prop(reader);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::ServerKeepAlive);

    uint16_t value = 0;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value, 300);
}

// Test decode uint32
TEST(Property, Decode_Uint32)
{
    const std::vector buffer = { std::byte{ 2 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x0E }, std::byte{ 0x10 } };

    auto reader = makeReader(buffer);
    const Property prop(reader);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::MessageExpiryInterval);

    uint32_t value = 0;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value, 3600);
}

// Test decode string
TEST(Property, Decode_String)
{
    const std::vector buffer = { std::byte{ 3 },   std::byte{ 0x00 }, std::byte{ 0x05 }, std::byte{ 'h' },
                                 std::byte{ 'e' }, std::byte{ 'l' },  std::byte{ 'l' },  std::byte{ 'o' } };

    auto reader = makeReader(buffer);
    const Property prop(reader);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::ContentType);

    std::string value;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value, "hello");
}

// Test decode binary
TEST(Property, Decode_Binary)
{
    const std::vector buffer
        = { std::byte{ 9 }, std::byte{ 0x00 }, std::byte{ 0x03 }, std::byte{ 0x01 }, std::byte{ 0x02 }, std::byte{ 0x03 } };

    auto reader = makeReader(buffer);
    const Property prop(reader);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::CorrelationData);

    std::vector<uint8_t> value;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value.size(), 3u);
    EXPECT_EQ(value[0], 0x01);
    EXPECT_EQ(value[1], 0x02);
    EXPECT_EQ(value[2], 0x03);
}

// Test decode UserProperty
TEST(Property, Decode_UserProperty)
{
    const std::vector buffer
        = { std::byte{ 38 },  std::byte{ 0x00 }, std::byte{ 0x03 }, std::byte{ 'k' }, std::byte{ 'e' },
            std::byte{ 'y' }, std::byte{ 0x00 }, std::byte{ 0x05 }, std::byte{ 'v' }, std::byte{ 'a' },
            std::byte{ 'l' }, std::byte{ 'u' },  std::byte{ 'e' } };

    auto reader = makeReader(buffer);
    const Property prop(reader);

    EXPECT_EQ(prop.getIdentifier(), PropertyIdentifier::UserProperty);

    std::pair<std::string, std::string> value;
    EXPECT_TRUE(prop.tryGetValue(value));
    EXPECT_EQ(value.first, "key");
    EXPECT_EQ(value.second, "value");
}

// Test round-trip encode/decode for all uint8 properties
TEST(Property, RoundTrip_AllUint8Properties)
{
    constexpr std::array uint8Props
        = { PayloadFormatIndicator,
            RequestProblemInformation,
            RequestResponseInformation,
            MaximumQoS,
            RetainAvailable,
            WildcardSubscriptionAvailable,
            SubscriptionIdentifierAvailable,
            SharedSubscriptionAvailable };

    for (auto id : uint8Props)
    {
        Property prop = makeProperty(id);

        EXPECT_EQ(prop.getIdentifier(), id);
        EXPECT_EQ(prop.getLength(), 2u);

        std::vector<std::byte> buffer;
        ByteWriter writer(buffer);
        prop.encode(writer);

        auto reader = makeReader(buffer);
        Property decoded(reader);

        EXPECT_EQ(decoded.getIdentifier(), id);
        uint8_t value = 0;
        EXPECT_TRUE(decoded.tryGetValue(value));
        EXPECT_EQ(value, 42);
    }
}

// Test tryGetValue failure cases (type mismatch)
TEST(Property, TryGetValue_TypeMismatch)
{
    const auto prop = Property::create<PayloadFormatIndicator, uint8_t>(1);

    uint8_t u8 = 0;
    EXPECT_TRUE(prop.tryGetValue(u8));
    EXPECT_EQ(u8, 1);

    uint16_t u16 = 0;
    EXPECT_FALSE(prop.tryGetValue(u16));

    uint32_t u32 = 0;
    EXPECT_FALSE(prop.tryGetValue(u32));

    std::string str;
    EXPECT_FALSE(prop.tryGetValue(str));

    std::vector<uint8_t> vec;
    EXPECT_FALSE(prop.tryGetValue(vec));

    std::pair<std::string, std::string> pair;
    EXPECT_FALSE(prop.tryGetValue(pair));
}

// Test toString for different types
TEST(Property, ToString_DifferentTypes)
{
    {
        const auto prop = Property::create<PayloadFormatIndicator, uint8_t>(255);
        EXPECT_EQ(prop.toString(), "255");
    }

    {
        const auto prop = Property::create<ServerKeepAlive, uint16_t>(1234);
        EXPECT_EQ(prop.toString(), "1234");
    }

    {
        const auto prop = Property::create<MessageExpiryInterval, uint32_t>(123456);
        EXPECT_EQ(prop.toString(), "123456");
    }

    {
        const auto prop = Property::create<ContentType, std::string>("text/plain");
        EXPECT_EQ(prop.toString(), "text/plain");
    }

    {
        const auto prop = Property::create<UserProperty, std::pair<std::string, std::string>>(std::make_pair("name", "value"));
        EXPECT_EQ(prop.toString(), "name, value");
    }
}

// Test equality operators
TEST(Property, EqualityOperators)
{
    const auto prop1 = Property::create<PayloadFormatIndicator, uint8_t>(1);
    const auto prop2 = Property::create<PayloadFormatIndicator, uint8_t>(1);
    const auto prop3 = Property::create<PayloadFormatIndicator, uint8_t>(2);
    const auto prop4 = Property::create<RequestProblemInformation, uint8_t>(1);

    EXPECT_TRUE(prop1 == prop2);
    EXPECT_FALSE(prop1 != prop2);

    EXPECT_FALSE(prop1 == prop3);
    EXPECT_TRUE(prop1 != prop3);

    EXPECT_FALSE(prop1 == prop4);
    EXPECT_TRUE(prop1 != prop4);
}

// Test all uint32 properties
TEST(Property, AllUint32Properties)
{
    const auto msg = Property::create<MessageExpiryInterval, uint32_t>(1000);
    const auto sub = Property::create<SubscriptionIdentifier, uint32_t>(2000);
    const auto ses = Property::create<SessionExpiryInterval, uint32_t>(3000);
    const auto wil = Property::create<WillDelayInterval, uint32_t>(4000);
    const auto max = Property::create<MaximumPacketSize, uint32_t>(5000);

    EXPECT_EQ(msg.getIdentifier(), PropertyIdentifier::MessageExpiryInterval);
    EXPECT_EQ(sub.getIdentifier(), PropertyIdentifier::SubscriptionIdentifier);
    EXPECT_EQ(ses.getIdentifier(), PropertyIdentifier::SessionExpiryInterval);
    EXPECT_EQ(wil.getIdentifier(), PropertyIdentifier::WillDelayInterval);
    EXPECT_EQ(max.getIdentifier(), PropertyIdentifier::MaximumPacketSize);
}

// Test all string properties
TEST(Property, AllStringProperties)
{
    const auto ct = Property::create<ContentType, std::string>("ct");
    const auto rt = Property::create<ResponseTopic, std::string>("rt");
    const auto ac = Property::create<AssignedClientIdentifier, std::string>("ac");
    const auto am = Property::create<AuthenticationMethod, std::string>("am");
    const auto ri = Property::create<ResponseInformation, std::string>("ri");
    const auto sr = Property::create<ServerReference, std::string>("sr");
    const auto rs = Property::create<ReasonString, std::string>("rs");

    EXPECT_EQ(ct.getIdentifier(), PropertyIdentifier::ContentType);
    EXPECT_EQ(rt.getIdentifier(), PropertyIdentifier::ResponseTopic);
    EXPECT_EQ(ac.getIdentifier(), PropertyIdentifier::AssignedClientIdentifier);
    EXPECT_EQ(am.getIdentifier(), PropertyIdentifier::AuthenticationMethod);
    EXPECT_EQ(ri.getIdentifier(), PropertyIdentifier::ResponseInformation);
    EXPECT_EQ(sr.getIdentifier(), PropertyIdentifier::ServerReference);
    EXPECT_EQ(rs.getIdentifier(), PropertyIdentifier::ReasonString);
}

// Test all uint16 properties
TEST(Property, AllUint16Properties)
{
    const auto sk = Property::create<ServerKeepAlive, uint16_t>(100);
    const auto rm = Property::create<ReceiveMaximum, uint16_t>(200);
    const auto tam = Property::create<TopicAliasMaximum, uint16_t>(300);
    const auto ta = Property::create<TopicAlias, uint16_t>(400);

    EXPECT_EQ(sk.getIdentifier(), PropertyIdentifier::ServerKeepAlive);
    EXPECT_EQ(rm.getIdentifier(), PropertyIdentifier::ReceiveMaximum);
    EXPECT_EQ(tam.getIdentifier(), PropertyIdentifier::TopicAliasMaximum);
    EXPECT_EQ(ta.getIdentifier(), PropertyIdentifier::TopicAlias);
}

// Test binary properties
TEST(Property, BinaryProperties)
{
    const std::vector<uint8_t> data1 = { 1, 2, 3 };
    const std::vector<uint8_t> data2 = { 4, 5, 6 };

    const auto cd = Property::create<CorrelationData, std::vector<uint8_t>>(data1);
    const auto ad = Property::create<AuthenticationData, std::vector<uint8_t>>(data2);

    EXPECT_EQ(cd.getIdentifier(), PropertyIdentifier::CorrelationData);
    EXPECT_EQ(ad.getIdentifier(), PropertyIdentifier::AuthenticationData);
}
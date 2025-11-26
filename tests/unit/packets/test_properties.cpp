//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "mqtt/packets/properties/properties.h"
#include "mqtt/packets/properties/property.h"
#include "mqtt/packets/properties/property_identifier.h"
#include "serialize/bytes.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

using namespace reactormq::mqtt::packets::properties;
using namespace reactormq::serialize;

static std::vector<std::byte> toVec(const std::initializer_list<unsigned int> bytes)
{
    std::vector<std::byte> v;
    v.reserve(bytes.size());
    for (const auto b : bytes)
    {
        v.push_back(std::byte{ static_cast<unsigned char>(b) });
    }
    return v;
}

TEST(Properties, DefaultConstructor)
{
    const Properties props;
    EXPECT_EQ(props.getProperties().size(), 0u);
    EXPECT_EQ(props.getLength(false), 0u);
    EXPECT_EQ(props.getLength(true), 1u);
}

TEST(Properties, ConstructorFromEmptyVector)
{
    const std::vector<Property> empty;
    const Properties props(empty);
    EXPECT_EQ(props.getProperties().size(), 0u);
    EXPECT_EQ(props.getLength(false), 0u);
    EXPECT_EQ(props.getLength(true), 1u);
}

TEST(Properties, ConstructorFromVector_SingleProperty)
{
    const auto prop = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 60 });
    const std::vector vec = { prop };

    const Properties props(vec);
    EXPECT_EQ(props.getProperties().size(), 1u);
    EXPECT_EQ(props.getLength(false), 3u);
    EXPECT_EQ(props.getLength(true), 4u);
}

TEST(Properties, ConstructorFromVector_MultipleProperties)
{
    const auto prop1 = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 60 });
    const auto prop2 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 2 });
    const std::vector vec = { prop1, prop2 };

    const Properties props(vec);
    EXPECT_EQ(props.getProperties().size(), 2u);

    const uint32_t expectedLength = prop1.getLength() + prop2.getLength();
    EXPECT_EQ(props.getLength(false), expectedLength);
}

TEST(Properties, GetLength_WithoutLengthField)
{
    const auto prop1 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 1 });
    const auto prop2 = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 120 });
    const std::vector vec = { prop1, prop2 };

    const Properties props(vec);

    const uint32_t expected = prop1.getLength() + prop2.getLength();
    EXPECT_EQ(props.getLength(false), expected);
}

TEST(Properties, GetLength_WithLengthField)
{
    const auto prop = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 1 });
    const std::vector vec = { prop };

    const Properties props(vec);

    const uint32_t contentLength = prop.getLength();
    const uint32_t vbiSize = variableByteIntegerSize(contentLength);
    EXPECT_EQ(props.getLength(true), contentLength + vbiSize);
}

TEST(Properties, GetLength_EmptyProperties_WithLengthField)
{
    const Properties props;
    EXPECT_EQ(props.getLength(true), 1u);
}

TEST(Properties, Encode_EmptyProperties)
{
    const Properties props;
    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);

    props.encode(writer);

    EXPECT_EQ(buffer.size(), 1u);
    EXPECT_EQ(buffer[0], std::byte{ 0x00 });
}

TEST(Properties, Encode_SingleProperty)
{
    const auto prop = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 2 });
    const std::vector vec = { prop };
    const Properties props(vec);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    props.encode(writer);

    EXPECT_EQ(buffer.size(), 3u);
    EXPECT_EQ(buffer[0], std::byte{ 0x02 });
    EXPECT_EQ(buffer[1], std::byte{ 0x24 });
    EXPECT_EQ(buffer[2], std::byte{ 0x02 });
}

TEST(Properties, Encode_MultipleProperties)
{
    const auto prop1 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 1 });
    const auto prop2 = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 60 });
    const std::vector vec = { prop1, prop2 };
    const Properties props(vec);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    props.encode(writer);

    EXPECT_EQ(buffer[0], std::byte{ 0x05 });
    EXPECT_EQ(buffer.size(), 6u);
}

TEST(Properties, Decode_EmptyProperties)
{
    const auto data = toVec({ 0x00 });
    ByteReader reader(data.data(), data.size());

    const Properties props(reader);
    EXPECT_EQ(props.getProperties().size(), 0u);
    EXPECT_TRUE(reader.isEof());
}

TEST(Properties, Decode_SingleProperty)
{
    const auto prop = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 2 });
    std::vector<std::byte> propBuffer;
    ByteWriter propWriter(propBuffer);
    prop.encode(propWriter);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    encodeVariableByteInteger(static_cast<uint32_t>(propBuffer.size()), writer);
    writer.writeBytes(propBuffer.data(), propBuffer.size());

    ByteReader reader(buffer.data(), buffer.size());
    const Properties props(reader);

    EXPECT_EQ(props.getProperties().size(), 1u);
    const auto decoded = props.getProperties()[0];
    EXPECT_EQ(decoded.getIdentifier(), PropertyIdentifier::MaximumQoS);

    uint8_t value = 0;
    EXPECT_TRUE(decoded.tryGetValue(value));
    EXPECT_EQ(value, 2u);
}

TEST(Properties, Decode_MultipleProperties)
{
    const auto prop1 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 1 });
    const auto prop2 = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 60 });

    std::vector<std::byte> prop1Buffer;
    ByteWriter prop1Writer(prop1Buffer);
    prop1.encode(prop1Writer);

    std::vector<std::byte> prop2Buffer;
    ByteWriter prop2Writer(prop2Buffer);
    prop2.encode(prop2Writer);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    const auto totalLength = static_cast<uint32_t>(prop1Buffer.size() + prop2Buffer.size());
    encodeVariableByteInteger(totalLength, writer);
    writer.writeBytes(prop1Buffer.data(), prop1Buffer.size());
    writer.writeBytes(prop2Buffer.data(), prop2Buffer.size());

    ByteReader reader(buffer.data(), buffer.size());
    const Properties props(reader);

    EXPECT_EQ(props.getProperties().size(), 2u);
    EXPECT_EQ(props.getProperties()[0].getIdentifier(), PropertyIdentifier::MaximumQoS);
    EXPECT_EQ(props.getProperties()[1].getIdentifier(), PropertyIdentifier::ServerKeepAlive);
}

TEST(Properties, RoundTrip_EmptyProperties)
{
    const Properties props1;

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    props1.encode(writer);

    ByteReader reader(buffer.data(), buffer.size());
    const Properties props2(reader);

    EXPECT_EQ(props2.getProperties().size(), 0u);
    EXPECT_EQ(props1, props2);
}

TEST(Properties, RoundTrip_SingleProperty)
{
    const auto prop = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 120 });
    const std::vector vec = { prop };
    const Properties props1(vec);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    props1.encode(writer);

    ByteReader reader(buffer.data(), buffer.size());
    const Properties props2(reader);

    EXPECT_EQ(props1, props2);
    EXPECT_EQ(props2.getProperties().size(), 1u);

    uint16_t value = 0;
    EXPECT_TRUE(props2.getProperties()[0].tryGetValue(value));
    EXPECT_EQ(value, 120u);
}

TEST(Properties, RoundTrip_MultipleProperties)
{
    const auto prop1 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 2 });
    const auto prop2 = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 60 });
    const auto prop3 = Property::create<PropertyIdentifier::SessionExpiryInterval>(uint32_t{ 3600 });
    const std::vector vec = { prop1, prop2, prop3 };
    const Properties props1(vec);

    std::vector<std::byte> buffer;
    ByteWriter writer(buffer);
    props1.encode(writer);

    ByteReader reader(buffer.data(), buffer.size());
    const Properties props2(reader);

    EXPECT_EQ(props1, props2);
    EXPECT_EQ(props2.getProperties().size(), 3u);
}

TEST(Properties, EqualityOperator_BothEmpty)
{
    const Properties props1;
    const Properties props2;
    EXPECT_EQ(props1, props2);
}

TEST(Properties, EqualityOperator_SameProperties)
{
    const auto prop1 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 1 });
    const auto prop2 = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 60 });

    const std::vector vec1 = { prop1, prop2 };
    const std::vector vec2 = { prop1, prop2 };

    const Properties props1(vec1);
    const Properties props2(vec2);

    EXPECT_EQ(props1, props2);
}

TEST(Properties, EqualityOperator_DifferentCount)
{
    const auto prop1 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 1 });
    const auto prop2 = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 60 });

    const std::vector vec1 = { prop1 };
    const std::vector vec2 = { prop1, prop2 };

    const Properties props1(vec1);
    const Properties props2(vec2);

    EXPECT_NE(props1, props2);
}

TEST(Properties, EqualityOperator_DifferentValues)
{
    const auto prop1 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 1 });
    const auto prop2 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 2 });

    const std::vector vec1 = { prop1 };
    const std::vector vec2 = { prop2 };

    const Properties props1(vec1);
    const Properties props2(vec2);

    EXPECT_NE(props1, props2);
}

TEST(Properties, GetProperties_ReturnsVector)
{
    const auto prop1 = Property::create<PropertyIdentifier::MaximumQoS>(uint8_t{ 1 });
    const auto prop2 = Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 60 });
    const std::vector vec = { prop1, prop2 };

    const Properties props(vec);
    const auto retrieved = props.getProperties();

    EXPECT_EQ(retrieved.size(), 2u);
    EXPECT_EQ(retrieved[0].getIdentifier(), PropertyIdentifier::MaximumQoS);
    EXPECT_EQ(retrieved[1].getIdentifier(), PropertyIdentifier::ServerKeepAlive);
}

TEST(Properties, LargePropertiesLength)
{
    std::vector<Property> vec;
    vec.reserve(50);
    for (int i = 0; i < 50; ++i)
    {
        vec.push_back(Property::create<PropertyIdentifier::ServerKeepAlive>(uint16_t{ 60 }));
    }

    const Properties props(vec);
    const uint32_t contentLength = props.getLength(false);
    EXPECT_GT(contentLength, 127u);

    const uint32_t totalLength = props.getLength(true);
    EXPECT_EQ(totalLength, contentLength + variableByteIntegerSize(contentLength));
}
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "serialize/bytes.h"
#include "serialize/mqtt_codec.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

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

TEST(VariableByteInteger, EncodeExamplesFromSpec)
{
    std::vector<std::byte> out;
    const ByteWriter writer(out);

    out.clear();
    encodeVariableByteInteger(0, writer);
    EXPECT_EQ(out, toVec({ 0x00 }));
    out.clear();
    encodeVariableByteInteger(127, writer);
    EXPECT_EQ(out, toVec({ 0x7F }));
    out.clear();
    encodeVariableByteInteger(128, writer);
    EXPECT_EQ(out, toVec({ 0x80, 0x01 }));
    out.clear();
    encodeVariableByteInteger(16383, writer);
    EXPECT_EQ(out, toVec({ 0xFF, 0x7F }));
    out.clear();
    encodeVariableByteInteger(16384, writer);
    EXPECT_EQ(out, toVec({ 0x80, 0x80, 0x01 }));
    out.clear();
    encodeVariableByteInteger(2097151, writer);
    EXPECT_EQ(out, toVec({ 0xFF, 0xFF, 0x7F }));
    out.clear();
    encodeVariableByteInteger(2097152, writer);
    EXPECT_EQ(out, toVec({ 0x80, 0x80, 0x80, 0x01 }));
    out.clear();
    encodeVariableByteInteger(268435455, writer);
    EXPECT_EQ(out, toVec({ 0xFF, 0xFF, 0xFF, 0x7F }));
}

TEST(VariableByteInteger, DecodeExamplesFromSpec)
{
    auto chk = [](const std::initializer_list<unsigned int> bytes, const uint32_t expect, const size_t expectConsumed)
    {
        const auto data = toVec(bytes);
        ByteReader reader(data.data(), data.size());
        const uint32_t value = decodeVariableByteInteger(reader);
        EXPECT_EQ(value, expect);
        EXPECT_EQ(data.size() - reader.getRemaining(), expectConsumed);
    };

    chk({ 0x00 }, 0u, 1u);
    chk({ 0x7F }, 127u, 1u);
    chk({ 0x80, 0x01 }, 128u, 2u);
    chk({ 0xFF, 0x7F }, 16383u, 2u);
    chk({ 0x80, 0x80, 0x01 }, 16384u, 3u);
    chk({ 0xFF, 0xFF, 0x7F }, 2097151u, 3u);
    chk({ 0x80, 0x80, 0x80, 0x01 }, 2097152u, 4u);
    chk({ 0xFF, 0xFF, 0xFF, 0x7F }, 268435455u, 4u);
}
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include <array>
#include <gtest/gtest.h>

#include "serialize/bytes.h"
#include "serialize/endian.h"

using namespace reactormq::serialize;

TEST(Serialize_Bytes, EndianHelpers)
{
    EXPECT_EQ(byteSwapUint16(0x1234u), 0x3412u);
    EXPECT_EQ(byteSwapUint32(0x11223344u), 0x44332211u);
    EXPECT_EQ(byteSwapUint64(0x0102030405060708ull), 0x0807060504030201ull);

    constexpr uint16_t v16 = 0xABCDu;
    constexpr uint32_t v32 = 0xA1B2C3D4u;
    constexpr uint64_t v64 = 0x0102030405060708ull;

    EXPECT_EQ(bigEndianToHost16(hostToBigEndian16(v16)), v16);
    EXPECT_EQ(bigEndianToHost32(hostToBigEndian32(v32)), v32);
    EXPECT_EQ(bigEndianToHost64(hostToBigEndian64(v64)), v64);
}

TEST(Serialize_Bytes, ByteWriterAndReader_Primitives)
{
    std::vector<std::byte> buf;
    ByteWriter w(buf);

    w.writeUint8(0x7Fu);
    w.writeUint16(0x1234u);
    w.writeUint32(0xA1B2C3D4u);
    w.writeUint64(0x0102030405060708ull);

    std::array blob{ std::byte{ 0xAA }, std::byte{ 0xBB }, std::byte{ 0xCC } };
    w.writeBytes(blob.data(), blob.size());

    ASSERT_EQ(buf.size(), 1 + 2 + 4 + 8 + 3);
    size_t i = 0;
    auto B = [&buf](const int idx)
    {
        return static_cast<unsigned int>(buf[idx]);
    };

    EXPECT_EQ(B(i++), 0x7Fu);

    EXPECT_EQ(B(i++), 0x12u);
    EXPECT_EQ(B(i++), 0x34u);

    EXPECT_EQ(B(i++), 0xA1u);
    EXPECT_EQ(B(i++), 0xB2u);
    EXPECT_EQ(B(i++), 0xC3u);
    EXPECT_EQ(B(i++), 0xD4u);

    EXPECT_EQ(B(i++), 0x01u);
    EXPECT_EQ(B(i++), 0x02u);
    EXPECT_EQ(B(i++), 0x03u);
    EXPECT_EQ(B(i++), 0x04u);
    EXPECT_EQ(B(i++), 0x05u);
    EXPECT_EQ(B(i++), 0x06u);
    EXPECT_EQ(B(i++), 0x07u);
    EXPECT_EQ(B(i++), 0x08u);

    EXPECT_EQ(B(i++), 0xAAu);
    EXPECT_EQ(B(i++), 0xBBu);
    EXPECT_EQ(B(i++), 0xCCu);

    ByteReader r(buf.data(), buf.size());
    EXPECT_EQ(r.readUint8(), 0x7Fu);
    EXPECT_EQ(r.readUint16(), 0x1234u);
    EXPECT_EQ(r.readUint32(), 0xA1B2C3D4u);
    EXPECT_EQ(r.readUint64(), 0x0102030405060708ull);

    std::byte out[3]{};
    r.readBytes(std::span{ out, 3 });
    EXPECT_EQ(out[0], std::byte{ 0xAA });
    EXPECT_EQ(out[1], std::byte{ 0xBB });
    EXPECT_EQ(out[2], std::byte{ 0xCC });

    EXPECT_TRUE(r.isEof());
}

TEST(Serialize_Bytes, ReaderBoundsChecks_NoExceptions)
{
    std::vector<std::byte> buf;
    const ByteWriter w(buf);
    w.writeUint32(0x01020304);

    ByteReader r(buf.data(), buf.size());

    uint16_t a = 0, b = 0;
    uint8_t c = 0;
    EXPECT_TRUE(r.tryReadUint16(a));
    EXPECT_TRUE(r.tryReadUint16(b));
    EXPECT_FALSE(r.tryReadUint8(c));
}
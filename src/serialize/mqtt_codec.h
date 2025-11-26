//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "bytes.h"
#include "util/logging/logging.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace reactormq::serialize
{
    namespace mqtt
    {
        static constexpr std::uint32_t kVariableByteBase = 128u;
        static constexpr std::uint32_t kVariableByteBits = 7u;
        static constexpr std::byte kContinuationFlag{ 0b1000'0000 };
        static constexpr std::byte kValueMask{
            static_cast<std::byte>(kVariableByteBase - 1u) // 0b0111'1111
        };
        static constexpr std::uint32_t kMaxVariableByteShift = 4u * kVariableByteBits;
    } // namespace mqtt

    /**
     * @brief Size in bytes needed to encode a variable byte integer (MQTT VBI).
     * @param value Non-negative value to be encoded.
     * @return 1-4 on success; 0 if value is out of range (>= 2^28).
     */
    inline uint8_t variableByteIntegerSize(const uint32_t value)
    {
        if (value < 1u << 7)
        {
            return 1;
        }
        if (value < 1u << 14)
        {
            return 2;
        }
        if (value < 1u << 21)
        {
            return 3;
        }
        if (value < 1u << 28)
        {
            return 4;
        }
        return 0;
    }

    /**
     * @brief Encode a variable byte integer (MQTT VBI) to a writer.
     * @param value Value to encode.
     * @param writer Destination writer.
     */
    inline void encodeVariableByteInteger(std::uint32_t value, const ByteWriter& writer)
    {
        do
        {
            std::byte byte{ static_cast<std::byte>(value % mqtt::kVariableByteBase) };
            value /= mqtt::kVariableByteBase;

            if (value > 0)
            {
                byte |= mqtt::kContinuationFlag;
            }

            writer.writeUint8(std::to_integer<std::uint8_t>(byte));
        } while (value > 0);
    }

    /**
     * @brief Decode a variable byte integer (MQTT VBI) from a reader.
     * @param reader Source reader.
     * @return Decoded value, or 0 on error (bounds or malformed input).
     */
    inline std::uint32_t decodeVariableByteInteger(ByteReader& reader)
    {
        std::uint32_t value = 0;
        std::uint32_t shift = 0;
        std::byte byte{ 0 };

        do
        {
            if (reader.isEof())
            {
                REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize");
                return 0;
            }

            byte = static_cast<std::byte>(reader.readUint8());

            const auto chunk = std::to_integer<std::uint32_t>(byte & mqtt::kValueMask);

            value |= (chunk << shift);
            shift += mqtt::kVariableByteBits;

            if (shift > mqtt::kMaxVariableByteShift)
            {
                REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize");
                return 0;
            }
        } while ((byte & mqtt::kContinuationFlag) != std::byte{ 0 });

        return value;
    }

    /**
     * @brief Encode a length-prefixed UTF-8 string.
     * @param str String to encode (assumed UTF-8, not validated).
     * @param writer Destination writer.
     * @return True on success; false if length exceeds 65535 bytes.
     */
    inline bool encodeString(const std::string_view str, const ByteWriter& writer)
    {
        if (str.size() > 65535)
        {
            REACTORMQ_LOG(
                reactormq::logging::LogLevel::Error,
                "Serialize EncodeString: String length %zu exceeds uint16 limit",
                str.size());
            return false;
        }

        const auto length = static_cast<uint16_t>(str.size());
        writer.writeUint16(length);
        writer.writeBytes(reinterpret_cast<const std::byte*>(str.data()), str.size());
        return true;
    }

    /**
     * @brief Decode a length-prefixed UTF-8 string.
     * @param reader Source reader.
     * @param outStr Output string (replaced on success).
     * @return True on success; false if not enough data or length invalid.
     */
    inline bool decodeString(ByteReader& reader, std::string& outStr)
    {
        uint16_t length = 0;
        if (!reader.tryReadUint16(length))
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize DecodeString: Failed to read string length");
            return false;
        }

        if (length == 0)
        {
            outStr.clear();
            return true;
        }

        outStr.resize(length);
        if (const auto span = std::span(reinterpret_cast<std::byte*>(outStr.data()), length); !reader.tryReadBytes(span))
        {
            outStr.clear();
            return false;
        }

        return true;
    }

    /**
     * @brief Append raw payload bytes.
     * @param data Pointer to data.
     * @param size Number of bytes.
     * @param writer Destination writer.
     */
    inline void encodePayload(const uint8_t* data, const size_t size, const ByteWriter& writer)
    {
        writer.writeBytes(reinterpret_cast<const std::byte*>(data), size);
    }

    /**
     * @brief Read raw payload bytes.
     * @param reader Source reader.
     * @param size Number of bytes to read.
     * @param outData Output buffer resized to size on success.
     * @return True on success; false if not enough data.
     */
    inline bool decodePayload(ByteReader& reader, const size_t size, std::vector<uint8_t>& outData)
    {
        if (reader.getRemaining() < size)
        {
            REACTORMQ_LOG(
                reactormq::logging::LogLevel::Error,
                "Serialize DecodePayload: Not enough data (need %zu, have %zu)",
                size,
                reader.getRemaining());
            return false;
        }

        outData.resize(size);

        const std::span dst{ reinterpret_cast<std::byte*>(outData.data()), outData.size() };

        reader.readBytes(dst);
        return true;
    }
} // namespace reactormq::serialize
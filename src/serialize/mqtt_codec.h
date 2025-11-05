#pragma once

#include "bytes.h"
#include "util/logging/logging.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace reactormq::serialize
{
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
    inline void encodeVariableByteInteger(uint32_t value, const ByteWriter& writer)
    {
        do
        {
            uint8_t byte = value % 128;
            value /= 128;

            if (value > 0)
            {
                byte |= 0x80;
            }

            writer.writeUint8(byte);
        } while (value > 0);
    }

    /**
     * @brief Decode a variable byte integer (MQTT VBI) from a reader.
     * @param reader Source reader.
     * @return Decoded value, or 0 on error (bounds or malformed input).
     */
    inline uint32_t decodeVariableByteInteger(ByteReader& reader)
    {
        uint32_t value = 0;
        uint8_t shift = 0;
        uint8_t byte = 0;

        do
        {
            if (reader.isEof())
            {
                REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize");
                return 0;
            }

            byte = reader.readUint8();
            value |= (byte & 0x7F) << shift;
            shift += 7;

            if (shift > 28)
            {
                REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize");
                return 0;
            }
        } while ((byte & 0x80) != 0);

        return value;
    }

    /**
     * @brief Encode a length-prefixed UTF-8 string.
     * @param str String to encode (assumed UTF-8, not validated).
     * @param writer Destination writer.
     * @return True on success; false if length exceeds 65535 bytes.
     */
    inline bool encodeString(const std::string& str, const ByteWriter& writer)
    {
        if (str.size() > 65535)
        {
            REACTORMQ_LOG(
                reactormq::logging::LogLevel::Error, "Serialize",
                "EncodeString: String length %zu exceeds uint16 limit", str.size());
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
            REACTORMQ_LOG(
                reactormq::logging::LogLevel::Error,
                "Serialize",
                "DecodeString: Failed to read string length");
            return false;
        }

        if (length == 0)
        {
            outStr.clear();
            return true;
        }

        outStr.resize(length);
        if (const auto span = std::span<std::byte>(
                reinterpret_cast<std::byte*>(outStr.data()),
                static_cast<size_t>(length));
            !reader.tryReadBytes(span))
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
                "Serialize",
                "DecodePayload: Not enough data (need %zu, have %zu)",
                size,
                reader.getRemaining());
            return false;
        }

        outData.resize(size);

        const std::span dst{
            reinterpret_cast<std::byte*>(outData.data()),
            outData.size()
        };

        reader.readBytes(dst);
        return true;
    }
} // namespace reactormq::serialize
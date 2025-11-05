#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include "bytes.h"
#include "util/logging/logging.h"

namespace reactormq::serialize
{
    /**
     * @brief Get the size of a variable byte integer.
     * @param value Integer to be encoded.
     * @return The size of the encoded integer (1-4 bytes). 0 if the integer is too large (>= 2^28).
     */
    inline uint8_t variableByteIntegerSize(const uint32_t value)
    {
        if (value < (1u << 7))
        {
            return 1;
        }
        if (value < (1u << 14))
        {
            return 2;
        }
        if (value < (1u << 21))
        {
            return 3;
        }
        if (value < (1u << 28))
        {
            return 4;
        }
        return 0;
    }

    /**
     * @brief Encode a variable byte integer to a ByteWriter.
     * @param value Integer to be encoded.
     * @param writer ByteWriter to write to.
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
        }
        while (value > 0);
    }

    /**
     * @brief Decode a variable byte integer from a ByteReader.
     * @param reader ByteReader to read from.
     * @return The decoded integer. Returns 0 on error (logged).
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
        }
        while (byte & 0x80);

        return value;
    }


    /**
     * @brief Encode a UTF-8 string with length prefix to a ByteWriter.
     * @param str String to encode (assumed UTF-8).
     * @param writer ByteWriter to write to.
     * @return true on success, false if string is too long (>65535 bytes).
     */
    inline bool encodeString(const std::string& str, const ByteWriter& writer)
    {
        if (str.size() > 65535)
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize", 
                "EncodeString: String length %zu exceeds uint16 limit", str.size());
            return false;
        }

        auto length = static_cast<uint16_t>(str.size());
        writer.writeUint16(length);
        writer.writeBytes(reinterpret_cast<const std::byte*>(str.data()), str.size());
        return true;
    }

    /**
     * @brief Decode a UTF-8 string with length prefix from a ByteReader.
     * @param reader ByteReader to read from.
     * @param outStr Output string.
     * @return true on success, false on error (logged).
     */
    inline bool decodeString(ByteReader& reader, std::string& outStr)
    {
        uint16_t length = 0;
        if (!reader.tryReadUint16(length))
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize");
            return false;
        }

        if (reader.getRemaining() < length)
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize", 
                "DecodeString: Not enough data (need %u, have %zu)", 
                length, reader.getRemaining());
            return false;
        }

        outStr.resize(length);
        reader.readBytes(reinterpret_cast<std::byte*>(outStr.data()), length);


        return true;
    }

    /**
     * @brief Encode raw binary data (payload) to a ByteWriter.
     * @param data Data pointer.
     * @param size Data size.
     * @param writer ByteWriter to write to.
     */
    inline void encodePayload(const uint8_t* data, const size_t size, const ByteWriter& writer)
    {
        writer.writeBytes(reinterpret_cast<const std::byte*>(data), size);
    }

    /**
     * @brief Decode raw binary data (payload) from a ByteReader.
     * @param reader ByteReader to read from.
     * @param size Number of bytes to read.
     * @param outData Output vector.
     * @return true on success, false on error (logged).
     */
    inline bool decodePayload(ByteReader& reader, const size_t size, std::vector<uint8_t>& outData)
    {
        if (reader.getRemaining() < size)
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize", 
                "DecodePayload: Not enough data (need %zu, have %zu)", 
                size, reader.getRemaining());
            return false;
        }

        outData.resize(size);
        reader.readBytes(reinterpret_cast<std::byte*>(outData.data()), size);
        return true;
    }

} // namespace reactormq::serialize

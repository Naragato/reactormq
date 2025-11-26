//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "endian.h"
#include "util/logging/logging.h"

namespace reactormq::serialize
{
    /**
     * @brief Appends primitive values and byte ranges to a growable buffer.
     * Writes big-endian integers and raw bytes into an external std::vector<std::byte>.
     * The writer does not fail; callers must size-check at a higher level when needed.
     */
    class ByteWriter
    {
    public:
        /**
         * @brief Construct a writer that appends to the provided buffer.
         * @param buffer Destination byte buffer (not owned).
         */
        explicit ByteWriter(std::vector<std::byte>& buffer)
            : m_buffer(buffer)
        {
        }

        /**
         * @brief Write an unsigned 8-bit value.
         */
        void writeUint8(const uint8_t v) const
        {
            writeBigEndian(v);
        }

        /**
         * @brief Write an unsigned 16-bit value in big-endian order.
         */
        void writeUint16(const uint16_t v) const
        {
            writeBigEndian(v);
        }

        /**
         * @brief Write an unsigned 32-bit value in big-endian order.
         */
        void writeUint32(const uint32_t v) const
        {
            writeBigEndian(v);
        }

        /**
         * @brief Write an unsigned 64-bit value in big-endian order.
         */
        void writeUint64(const uint64_t v) const
        {
            writeBigEndian(v);
        }

        /**
         * @brief Append a raw byte range.
         * @param data Pointer to bytes; may be null when size is 0.
         * @param size Number of bytes to append.
         */
        void writeBytes(const std::byte* data, const size_t size) const
        {
            if (size == 0)
            {
                return;
            }
            m_buffer.insert(m_buffer.end(), data, data + size);
        }

        /**
         * @brief Current number of bytes in the destination buffer.
         */
        [[nodiscard]] size_t getSize() const
        {
            return m_buffer.size();
        }

    private:
        template<typename T>
        void writeBigEndian(const T v) const
        {
            static_assert(std::is_unsigned_v<T>, "ByteWriter only supports unsigned integer types");
            static_assert(sizeof(T) <= sizeof(uint64_t), "Unsupported integer size");

            const auto value = static_cast<uint64_t>(v);
            for (size_t i = 0; i < sizeof(T); ++i)
            {
                const size_t shift = (sizeof(T) - 1 - i) * 8;
                const auto byteValue = static_cast<std::byte>(value >> shift & 0xFFu);
                m_buffer.push_back(byteValue);
            }
        }

        std::vector<std::byte>& m_buffer;
    };

    /**
     * @brief Reads big-endian integers and byte ranges from a fixed buffer.
     *
     * Safe helpers return default values and log when reads would exceed bounds.
     */
    class ByteReader
    {
    public:
        /**
         * @brief Construct a reader over a contiguous byte buffer.
         * @param data Pointer to the first byte.
         * @param size Total bytes available from data.
         */
        ByteReader(const std::byte* data, const size_t size)
            : m_data(data)
            , m_size(size)
        {
        }

        /**
         * @brief Construct a reader over a contiguous byte buffer.
         * @param buffer the data to read
         */
        explicit ByteReader(const std::span<const std::byte>& buffer)
            : ByteReader(buffer.data(), buffer.size())
        {
        }

        /// @brief True when all bytes have been consumed.
        [[nodiscard]] bool isEof() const
        {
            return m_pos == m_size;
        }

        /// @brief Bytes remaining that can still be read.
        [[nodiscard]] size_t getRemaining() const
        {
            return m_pos <= m_size ? m_size - m_pos : 0;
        }

        /// @brief Total buffer size.
        [[nodiscard]] size_t getSize() const
        {
            return m_size;
        }

        /**
         * @brief Read an unsigned 8-bit value; returns 0 on out-of-bounds.
         */
        uint8_t readUint8()
        {
            uint8_t v = 0;
            if (!tryReadUint8(v))
            {
                // tryReadUint8 already logged OOB
                return 0;
            }
            return v;
        }

        /**
         * @brief Read an unsigned 16-bit big-endian value; returns 0 on out-of-bounds.
         */
        uint16_t readUint16()
        {
            uint16_t v = 0;
            if (!tryReadUint16(v))
            {
                return 0;
            }
            return v;
        }

        /**
         * @brief Read an unsigned 32-bit big-endian value; returns 0 on out-of-bounds.
         */
        uint32_t readUint32()
        {
            uint32_t v = 0;
            if (!tryReadUint32(v))
            {
                return 0;
            }
            return v;
        }

        /**
         * @brief Read an unsigned 64-bit big-endian value; returns 0 on out-of-bounds.
         */
        uint64_t readUint64()
        {
            uint64_t v = 0;
            if (!tryReadUint64(v))
            {
                return 0;
            }
            return v;
        }

        /**
         * @brief Read a raw byte range into the destination; no-op on out-of-bounds.
         * @param out Destination span with capacity for the requested bytes.
         */
        void readBytes(std::span<std::byte> out)
        {
            const size_t size = out.size();
            if (!available(size))
            {
                logBounds("blob");
                return;
            }
            for (size_t i = 0; i < size; ++i)
            {
                out[i] = m_data[m_pos + i];
            }
            m_pos += size;
        }

        /**
         * @brief Try to read a u8; returns false if not enough data.
         */
        bool tryReadUint8(uint8_t& out)
        {
            return tryReadBigEndian<uint8_t>(out, "u8");
        }

        /**
         * @brief Try to read a big-endian u16; returns false if not enough data.
         */
        bool tryReadUint16(uint16_t& out)
        {
            return tryReadBigEndian<uint16_t>(out, "u16");
        }

        /**
         * @brief Try to read a big-endian u32; returns false if not enough data.
         */
        bool tryReadUint32(uint32_t& out)
        {
            return tryReadBigEndian<uint32_t>(out, "u32");
        }

        /**
         * @brief Try to read a big-endian u64; returns false if not enough data.
         */
        bool tryReadUint64(uint64_t& out)
        {
            return tryReadBigEndian<uint64_t>(out, "u64");
        }

        /**
         * @brief Try to read a raw byte range.
         * @param dst Destination span with capacity for the requested bytes.
         * @return True on success; false if not enough data.
         */
        bool tryReadBytes(std::span<std::byte> dst)
        {
            const size_t size = dst.size();
            if (!available(size))
            {
                logBounds("blob");
                return false;
            }
            for (size_t i = 0; i < size; ++i)
            {
                dst[i] = m_data[m_pos + i];
            }
            m_pos += size;
            return true;
        }

    private:
        /// @brief Check whether n bytes can be read without overrunning the buffer.
        [[nodiscard]] bool available(const size_t n) const
        {
            return m_pos + n <= m_size;
        }

        /**
         * @brief Try to read an unsigned integer in big-endian order.
         *
         * Reads @c sizeof(T) bytes starting at the current read position,
         * interpreting them as a big-endian unsigned integer, and converts the
         * result into the host's native integer representation.
         *
         * On success, advances @ref m_pos by @c sizeof(T), assigns the decoded
         * value to @p out, and returns true. On failure, logs an out-of-bounds
         * error with the provided tag and returns false without modifying @p out.
         *
         * @tparam T Unsigned integer type to read (e.g. uint8_t, uint16_t, uint32_t, uint64_t).
         * @param out Reference to receive the decoded value.
         * @param what Short tag used in the out-of-bounds log message.
         * @return True if enough bytes were available and the value was read; false otherwise.
         */
        template<typename T>
        bool tryReadBigEndian(T& out, const char* what)
        {
            static_assert(std::is_unsigned_v<T>, "ByteReader only supports unsigned integer types");
            static_assert(sizeof(T) <= sizeof(uint64_t), "Unsupported integer size");

            const size_t n = sizeof(T);
            if (!available(n))
            {
                logBounds(what);
                return false;
            }

            uint64_t acc = 0;
            for (size_t i = 0; i < n; ++i)
            {
                acc = (acc << 8) | static_cast<uint64_t>(static_cast<uint8_t>(m_data[m_pos + i]));
            }
            m_pos += n;
            out = static_cast<T>(acc);
            return true;
        }

        /// @brief Log an out-of-bounds read attempt with a short tag.
        void logBounds(const char* what) const
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "ByteReader: OOB while reading %s at %zu/%zu", what, m_pos, m_size);
        }

        const std::byte* m_data;
        size_t m_size;
        size_t m_pos{ 0 };
    };
} // namespace reactormq::serialize
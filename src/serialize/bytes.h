#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "endian.h"
#include "util/logging/logging.h"

namespace reactormq::serialize
{
    class ByteWriter
    {
    public:
        explicit ByteWriter(std::vector<std::byte> &buffer)
            : m_buffer(buffer)
        {
        }

        void writeUint8(uint8_t v)const
        {
            m_buffer.push_back(static_cast<std::byte>(v));
        }

        void writeUint16(const uint16_t v)const
        {
            uint16_t be = htobe16(v);
            const auto *p = reinterpret_cast<const uint8_t *>(&be);
            m_buffer.push_back(static_cast<std::byte>(p[0]));
            m_buffer.push_back(static_cast<std::byte>(p[1]));
        }

        void writeUint32(const uint32_t v)const
        {
            const uint32_t be = htobe32(v);
            const auto *p = reinterpret_cast<const uint8_t *>(&be);
            m_buffer.push_back(static_cast<std::byte>(p[0]));
            m_buffer.push_back(static_cast<std::byte>(p[1]));
            m_buffer.push_back(static_cast<std::byte>(p[2]));
            m_buffer.push_back(static_cast<std::byte>(p[3]));
        }

        void writeUint64(const uint64_t v)const
        {
            const uint64_t be = htobe64(v);
            const auto *p = reinterpret_cast<const uint8_t *>(&be);
            for (size_t i = 0; i < 8; ++i)
            {
                m_buffer.push_back(static_cast<std::byte>(p[i]));
            }
        }

        void writeBytes(const std::byte *data, const size_t size)const
        {
            if (size == 0)
            {
                return;
            }
            m_buffer.insert(m_buffer.end(), data, data + size);
        }

        [[nodiscard]] size_t getSize() const { return m_buffer.size(); }

    private:
        std::vector<std::byte> &m_buffer;
    };

    class ByteReader
    {
    public:
        ByteReader(const std::byte *data, const size_t size)
            : m_data(data), m_size(size), m_pos(0)
        {
        }

        [[nodiscard]] bool isEof() const { return m_pos == m_size; }
        [[nodiscard]] size_t getRemaining() const { return (m_pos <= m_size) ? (m_size - m_pos) : 0; }
        [[nodiscard]] size_t getSize() const { return m_size; }

        uint8_t readUint8()
        {
            if (!available(1))
            {
                logBounds("u8");
                return 0;
            }
            return static_cast<uint8_t>(m_data[m_pos++]);
        }

        uint16_t readUint16()
        {
            if (!available(2))
            {
                logBounds("u16");
                return 0;
            }
            uint16_t v = (static_cast<uint16_t>(static_cast<uint8_t>(m_data[m_pos])) << 8) |
                          (static_cast<uint16_t>(static_cast<uint8_t>(m_data[m_pos + 1])));
            m_pos += 2;
            return v;
        }

        uint32_t readUint32()
        {
            if (!available(4))
            {
                logBounds("u32");
                return 0;
            }
            uint32_t v = (static_cast<uint32_t>(static_cast<uint8_t>(m_data[m_pos])) << 24) |
                          (static_cast<uint32_t>(static_cast<uint8_t>(m_data[m_pos + 1])) << 16) |
                          (static_cast<uint32_t>(static_cast<uint8_t>(m_data[m_pos + 2])) << 8) |
                          (static_cast<uint32_t>(static_cast<uint8_t>(m_data[m_pos + 3])));
            m_pos += 4;
            return v;
        }

        uint64_t readUint64()
        {
            if (!available(8))
            {
                logBounds("u64");
                return 0;
            }
            uint64_t v = 0;
            for (size_t i = 0; i < 8; ++i)
            {
                v = (v << 8) | static_cast<uint64_t>(static_cast<uint8_t>(m_data[m_pos + i]));
            }
            m_pos += 8;
            return v;
        }

        void readBytes(std::byte *out, const size_t size)
        {
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

        bool tryReadUint8(uint8_t &out)
        {
            if (!available(1))
            {
                logBounds("u8");
                return false;
            }
            out = static_cast<uint8_t>(m_data[m_pos++]);
            return true;
        }

        bool tryReadUint16(uint16_t &out)
        {
            if (!available(2))
            {
                logBounds("u16");
                return false;
            }
            out = (static_cast<uint16_t>(static_cast<uint8_t>(m_data[m_pos])) << 8) |
                  (static_cast<uint16_t>(static_cast<uint8_t>(m_data[m_pos + 1])));
            m_pos += 2;
            return true;
        }

        bool tryReadUint32(uint32_t &out)
        {
            if (!available(4))
            {
                logBounds("u32");
                return false;
            }
            out = (static_cast<uint32_t>(static_cast<uint8_t>(m_data[m_pos])) << 24) |
                  (static_cast<uint32_t>(static_cast<uint8_t>(m_data[m_pos + 1])) << 16) |
                  (static_cast<uint32_t>(static_cast<uint8_t>(m_data[m_pos + 2])) << 8) |
                  (static_cast<uint32_t>(static_cast<uint8_t>(m_data[m_pos + 3])));
            m_pos += 4;
            return true;
        }

        bool tryReadUint64(uint64_t &out)
        {
            if (!available(8))
            {
                logBounds("u64");
                return false;
            }
            uint64_t v = 0;
            for (size_t i = 0; i < 8; ++i)
            {
                v = (v << 8) | static_cast<uint64_t>(static_cast<uint8_t>(m_data[m_pos + i]));
            }
            m_pos += 8;
            out = v;
            return true;
        }

        bool tryReadBytes(std::byte *dst, const size_t size)
        {
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
        [[nodiscard]] bool available(const size_t n) const
        {
            return m_pos + n <= m_size;
        }

        void logBounds(const char *what)const
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Error, "Serialize", "ByteReader: OOB while reading %s at %zu/%zu", what, m_pos, m_size);
        }

        const std::byte *m_data;
        size_t m_size;
        size_t m_pos;
    };
}

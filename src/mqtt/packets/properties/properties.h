//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "mqtt/packets/properties/property.h"
#include "serialize/bytes.h"
#include "serialize/mqtt_codec.h"
#include "util/logging/logging.h"

#include <cstdint>
#include <vector>

namespace reactormq::mqtt::packets::properties
{
    /**
     * @brief Container for MQTT properties.
     * Represents the properties section of an MQTT packet.
     */
    class Properties
    {
    public:
        Properties() = default;

        /**
         * @brief Constructor from vector of properties.
         * @param properties Vector of Property objects.
         */
        explicit Properties(std::vector<Property> properties)
            : m_properties(std::move(properties))
        {
        }

        /**
         * @brief Constructor that decodes from a ByteReader.
         * @param reader ByteReader to decode from.
         */
        explicit Properties(serialize::ByteReader& reader)
        {
            decode(reader);
        }

        /**
         * @brief Equality operator.
         * @param other Properties to compare with.
         * @return true if equal, false otherwise.
         */
        bool operator==(const Properties& other) const
        {
            if (m_properties.size() != other.m_properties.size())
            {
                return false;
            }

            for (size_t i = 0; i < m_properties.size(); ++i)
            {
                if (m_properties[i] != other.m_properties[i])
                {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Encode the properties to a ByteWriter.
         * @param writer ByteWriter to write to.
         */
        void encode(serialize::ByteWriter& writer) const
        {
            serialize::encodeVariableByteInteger(getLength(false), writer);

            for (const Property& prop : m_properties)
            {
                prop.encode(writer);
            }
        }

        /**
         * @brief Decode the properties from a ByteReader.
         * @param reader ByteReader to read from.
         */
        void decode(serialize::ByteReader& reader)
        {
            uint32_t remainingLength = serialize::decodeVariableByteInteger(reader);

            size_t readerPosBefore = reader.getRemaining();

            while (remainingLength > 0)
            {
                Property prop(reader);

                const size_t readerPosAfter = reader.getRemaining();

                if (readerPosBefore == readerPosAfter)
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Error,
                        "Failed to read property. Expected remaining: %u, Actual consumed: 0",
                        remainingLength);
                    return;
                }

                readerPosBefore = readerPosAfter;
                const uint32_t propLength = prop.getLength();
                if (propLength > remainingLength)
                {
                    REACTORMQ_LOG(logging::LogLevel::Error, "Property length %u exceeds remaining length %u", propLength, remainingLength);
                    return;
                }
                m_properties.emplace_back(std::move(prop));
                remainingLength -= propLength;
            }
        }

        /**
         * @brief Get the length of the properties.
         * @param withLengthField Whether to include the VBI length field in the calculation.
         * @return The length of the properties in bytes.
         */
        [[nodiscard]] uint32_t getLength(const bool withLengthField = true) const
        {
            uint32_t totalLength = 0;

            for (const Property& prop : m_properties)
            {
                totalLength += prop.getLength();
            }

            if (withLengthField)
            {
                if (totalLength > 0)
                {
                    totalLength += serialize::variableByteIntegerSize(totalLength);
                }
                else
                {
                    totalLength += 1;
                }
            }

            return totalLength;
        }

        /**
         * @brief Get the vector of properties.
         * @return Vector of Property objects.
         */
        [[nodiscard]] std::vector<Property> getProperties() const
        {
            return m_properties;
        }

    private:
        std::vector<Property> m_properties;
    };

    /**
     * @brief Empty properties placeholder for MQTT versions without properties.
     */
    struct EmptyProperties
    {
    };
} // namespace reactormq::mqtt::packets::properties
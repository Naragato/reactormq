//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "bytes.h"

namespace reactormq::serialize
{
    /**
     * @brief Interface for types that can be encoded to and decoded from bytes.
     * Implementations write their payload using the provided writer and must be
     * able to restore state from a reader. Decode returns false if the input
     * does not contain a valid representation.
     */
    struct ISerializable
    {
        virtual ~ISerializable() = default;

        /**
         * @brief Encode this object to the provided writer.
         * @param w Destination writer.
         */
        virtual void encode(ByteWriter& w) const = 0;

        /**
         * @brief Decode this object from the provided reader.
         * @param r Source reader positioned at the start of this object.
         * @return True on success; false if the data is invalid or incomplete.
         */
        virtual bool decode(ByteReader& r) = 0;
    };
} // namespace reactormq::serialize
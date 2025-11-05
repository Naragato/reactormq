#pragma once

#include "bytes.h"

namespace reactormq::serialize
{
    struct ISerializable
    {
        virtual ~ISerializable() = default;

        /**
         *
         * @param w Writer to encode
         */
        virtual void encode(ByteWriter &w) const = 0;

        /**
         *
         * @param r Reader to decode
         * @return false on fail to decode
         */
        virtual bool decode(ByteReader &r) = 0;
    };
} // namespace reactormq::serialize

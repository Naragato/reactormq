//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "serialize/bytes.h"
#include "serialize/mqtt_codec.h"

#include <cstddef>

using reactormq::serialize::ByteReader;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size)
{
    if (data == nullptr || size == 0)
    {
        return 0;
    }

    const std::span buffer{ reinterpret_cast<const std::byte*>(data), size };
    ByteReader reader(buffer);

    (void)reactormq::serialize::decodeVariableByteInteger(reader);

    while (!reader.isEof())
    {
        (void)reactormq::serialize::decodeVariableByteInteger(reader);
    }

    return 0;
}

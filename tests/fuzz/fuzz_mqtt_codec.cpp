//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "serialize/bytes.h"
#include "serialize/mqtt_codec.h"

#include <cstddef>
#include <string>
#include <vector>

using reactormq::serialize::ByteReader;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size)
{
    if (data == nullptr || size < 2)
    {
        return 0;
    }

    const std::span buffer{ reinterpret_cast<const std::byte*>(data), size };
    ByteReader reader(buffer);

    std::string decodedString;
    (void)reactormq::serialize::decodeString(reader, decodedString);

    ByteReader payloadReader(buffer);

    if (payloadReader.getRemaining() > 0)
    {
        const auto payloadSize = payloadReader.getRemaining() / 2 + 1;
        std::vector<std::uint8_t> payload;
        (void)reactormq::serialize::decodePayload(payloadReader, payloadSize, payload);
    }

    return 0;
}

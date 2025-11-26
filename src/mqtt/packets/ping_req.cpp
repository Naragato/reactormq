//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "ping_req.h"

namespace reactormq::mqtt::packets
{
    void PingReq::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][PingReq]");
        getFixedHeader().encode(writer);
    }

    bool PingReq::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][PingReq]");
        (void)reader;
        return true;
    }
} // namespace reactormq::mqtt::packets
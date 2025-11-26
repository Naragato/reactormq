//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "ping_resp.h"

namespace reactormq::mqtt::packets
{
    void PingResp::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][PingResp]");
        getFixedHeader().encode(writer);
    }

    bool PingResp::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][PingResp]");
        (void)reader;
        return true;
    }
} // namespace reactormq::mqtt::packets
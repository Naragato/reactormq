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
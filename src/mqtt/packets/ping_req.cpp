#include "ping_req.h"

namespace reactormq::mqtt::packets
{
    void PingReq::encode(serialize::ByteWriter& writer) const
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Encode][PingReq]");
        m_fixedHeader.encode(writer);
    }

    bool PingReq::decode(serialize::ByteReader& reader)
    {
        REACTORMQ_LOG(logging::LogLevel::Trace, "[Decode][PingReq]");
        (void)reader;
        return true;
    }
} // namespace reactormq::mqtt::packets
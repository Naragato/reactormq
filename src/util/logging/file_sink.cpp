#include "file_sink.h"
#include "console_sink.h" // for to_iso8601_ms/thread_id_str helpers

#include <iomanip>

namespace reactormq::logging
{
    FileSink::FileSink(const std::string& path)
    {
        out_.open(path, std::ios::out | std::ios::app);
    }

    FileSink::~FileSink()
    {
        if (out_.is_open())
            out_.close();
    }

    void FileSink::log(const LogMessage& msg)
    {
        std::scoped_lock lock(mtx_);
        if (!out_.is_open())
            return;

        const auto ts = ConsoleSink::to_iso8601_ms(msg.timestamp);
        const auto tid = ConsoleSink::thread_id_str(msg.thread_id);

        out_ << ts << " | " << tid
            << " | " << level_to_string(msg.level)
            << " | " << msg.function
            << " | " << msg.file << ':' << msg.line
            << " | " << msg.text << '\n';
        out_.flush();
    }
} // namespace reactormq::logging
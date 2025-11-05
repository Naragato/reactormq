#pragma once

#include "logging.h"
#include <fstream>
#include <mutex>

namespace reactormq
{
    namespace logging
    {
        class FileSink : public Sink
        {
        public:
            explicit FileSink(const std::string &path = "reactormq.log");

            ~FileSink() override;

            void log(const LogMessage &msg) override;

        private:
            std::ofstream out_;
            std::mutex mtx_;
        };
    }
} // namespace reactormq::logging
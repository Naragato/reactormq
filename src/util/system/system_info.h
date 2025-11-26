//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <string>

#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
#include <cstdlib>
#include <process.h>
#else // REACTORMQ_PLATFORM_WINDOWS_FAMILY
#include <algorithm>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#endif // REACTORMQ_PLATFORM_WINDOWS_FAMILY

namespace reactormq::system
{
    inline std::string getComputerName()
    {
#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
#ifdef _MSC_VER
        char* duplicated = nullptr;
        size_t length = 0;
        if (_dupenv_s(&duplicated, &length, "COMPUTERNAME") == 0 && duplicated != nullptr)
        {
            std::string value;
            if (length > 1 && duplicated[0] != '\0')
            {
                value.assign(duplicated);
            }
            free(duplicated);
            if (!value.empty())
            {
                return value;
            }
        }
#else
        if (const char* name = std::getenv("COMPUTERNAME"))
        {
            if (*name != '\0')
            {
                return std::string{ name };
            }
        }
#endif
        return "REACTORMQ";
#else
        long hostNameMax = sysconf(_SC_HOST_NAME_MAX);
        if (hostNameMax <= 0)
        {
            hostNameMax = 255;
        }

        std::string name;
        name.resize(static_cast<size_t>(hostNameMax) + 1); // +1 for NUL

#if REACTORMQ_WITH_GETHOSTNAME
        if (gethostname(name.data(), name.size()) != 0)
        {
            return "REACTORMQ";
        }
#endif // REACTORMQ_WITH_GETHOSTNAME

        name.back() = '\0';
        const auto nul = std::ranges::find(name, '\0');
        name.resize(static_cast<size_t>(nul - name.begin()));

        return name.empty() ? "REACTORMQ" : name;
#endif
    }

    inline unsigned long getProcessId()
    {
#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
        return static_cast<unsigned long>(::_getpid());
#else
        return static_cast<unsigned long>(::getpid());
#endif
    }
} // namespace reactormq::system
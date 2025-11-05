#pragma once

#include <string>
#include <stdexcept>

#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#include <processthreadsapi.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <cstring>
#endif

namespace reactormq::system
{
    inline std::string getComputerName()
    {
#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
        WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        if (GetComputerNameW(buffer, &size) == 0)
        {
            return "REACTORMQ";
        }
        std::string result;
        result.reserve(size);
        for (DWORD i = 0; i < size; ++i)
        {
            result.push_back(static_cast<char>(buffer[i] & 0xFF));
        }
        return result;
#else
        long hostNameMax = sysconf(_SC_HOST_NAME_MAX); if (hostNameMax <= 0)
        {
            hostNameMax = 255;
        } std::string name; name.resize(static_cast<std::size_t>(hostNameMax) + 1);
#ifdef gethostname
        if (gethostname(name.data(), name.size()) != 0)
#endif // gethostname
        {
            return "REACTORMQ";
        } name.resize(std::strlen(name.c_str())); return name;
#endif
    }

    inline unsigned long getProcessId()
    {
#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
        return static_cast<unsigned long>(::GetCurrentProcessId());
#else
        return static_cast<unsigned long>(::getpid());
#endif
    }
} // reactormq::system
#include "fixtures/docker_container.h"

#include <array>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <thread>
#include <util/logging/logging.h>

using namespace std::chrono_literals;

namespace reactormq::test
{
    std::string DockerContainer::trim(std::string s)
    {
        const auto notSpace = [](const unsigned char c)
        {
            return !std::isspace(c);
        };
        s.erase(s.begin(), std::ranges::find_if(s, notSpace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
        return s;
    }

    std::pair<int, std::string> DockerContainer::runGetOutput(const std::string& cmd)
    {
#if defined(_WIN32)
        std::string full = cmd + " 2>&1";
        FILE* pipe = _popen(full.c_str(), "r");
#else
        std::string full = cmd + " 2>&1";
        FILE* pipe = popen(full.c_str(), "r");
#endif
        if (!pipe)
        {
            return { -1, std::string() };
        }
        std::array<char, 256> buffer{};
        std::ostringstream output;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        {
            output << buffer.data();
        }
#if defined(_WIN32)
        int code = _pclose(pipe);
#else
        int code = pclose(pipe);
#endif
        return { code, trim(output.str()) };
    }

    int DockerContainer::run(const std::string& cmd)
    {
        return std::system(cmd.c_str());
    }

    bool DockerContainer::isDockerAvailable()
    {
        auto [code, out] = runGetOutput("docker --version");
        REACTORMQ_LOG(
            logging::LogLevel::Debug,
            "[isDockerAvailable] code=%d output=\"%s\" path=\"%s\"",
            code, out.c_str(),
            std::getenv("PATH"));

        return code == 0;
    }

    bool DockerContainer::waitForRunning(const std::chrono::seconds timeoutSeconds) const
    {
        const auto deadline = std::chrono::steady_clock::now() + timeoutSeconds;
        while (std::chrono::steady_clock::now() < deadline)
        {
            auto [code, out] = runGetOutput("docker inspect -f \"{{.State.Running}}\" " + m_name);
            REACTORMQ_LOG(logging::LogLevel::Debug, "[waitForRunning] code=%d output=\"%s\"", code, out.c_str());
            if (code == 0 && (out == "true" || out == "True"))
            {
                return true;
            }
            std::this_thread::sleep_for(200ms);
        }
        return false;
    }

    std::future<bool> DockerContainer::startAsync(std::chrono::seconds timeoutSeconds)
    {
        return std::async(
            std::launch::async, [this, timeoutSeconds]() -> bool
            {
                if (!isDockerAvailable())
                {
                    return false;
                }

                // best-effort removal of any previous container with same name
#if defined(_WIN32)
                run("docker rm -f " + m_name + " >nul 2>&1");
#else
                run("docker rm -f " + m_name + " >/dev/null 2>&1");
#endif

                std::ostringstream runCmd;
                runCmd << "docker run --name " << m_name
                    << " -d -p " << m_hostPort << ':' << m_containerPort;
                if (!m_runArgs.empty())
                {
                    runCmd << ' ' << m_runArgs;
                }
                runCmd << ' ' << m_image;

                int code = run(runCmd.str());
                if (code != 0)
                {
                    return false;
                }

                if (!waitForRunning(timeoutSeconds))
                {
                    return false;
                }

                m_isRunning.store(true);
                return true;
            });
    }

    void DockerContainer::stop()
    {
        if (!m_isRunning.load())
        {
            return;
        }
        // stop and remove quietly
#if defined(_WIN32)
        run("docker stop " + m_name + " >nul 2>&1");
        run("docker rm " + m_name + " >nul 2>&1");
#else
        run("docker stop " + m_name + " >/dev/null 2>&1");
        run("docker rm " + m_name + " >/dev/null 2>&1");
#endif
        m_isRunning.store(false);
    }
}
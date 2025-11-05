#include "fixtures/docker_container.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <thread>
#include <util/logging/logging.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#endif

using namespace std::chrono_literals;

namespace reactormq::test
{
    std::string DockerContainer::trim(std::string s)
    {
        const auto originalSize = s.size();
        const auto notSpace = [](const unsigned char c)
        {
            return !std::isspace(c);
        };
        s.erase(s.begin(), std::ranges::find_if(s, notSpace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());

        REACTORMQ_LOG(
            logging::LogLevel::Debug, "[trim] original_size=%zu trimmed_size=%zu", static_cast<std::size_t>(originalSize),
            static_cast<std::size_t>(s.size()));

        return s;
    }

    std::pair<int, std::string> DockerContainer::runGetOutput(const std::string& cmd)
    {
        REACTORMQ_LOG(logging::LogLevel::Debug, "[runGetOutput] executing command=\"%s\"", cmd.c_str());

#if defined(_WIN32)
        const std::string full = cmd + " 2>&1";
        FILE* pipe = _popen(full.c_str(), "r");
#else
        const std::string full = cmd + " 2>&1"; FILE* pipe = popen(full.c_str(), "r");
#endif
        if (nullptr == pipe)
        {
            REACTORMQ_LOG(logging::LogLevel::Error, "[runGetOutput] failed to open pipe for command=\"%s\"", full.c_str());
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

        const auto outStr = trim(output.str());
        REACTORMQ_LOG(
            logging::LogLevel::Debug, "[runGetOutput] command=\"%s\" exit_code=%d output=\"%s\"", cmd.c_str(), code, outStr.c_str());

        return { code, outStr };
    }

    int DockerContainer::run(const std::string& cmd)
    {
        REACTORMQ_LOG(logging::LogLevel::Debug, "[run] executing command=\"%s\"", cmd.c_str());

        const int code = std::system(cmd.c_str());

        REACTORMQ_LOG(logging::LogLevel::Debug, "[run] command=\"%s\" exit_code=%d", cmd.c_str(), code);

        return code;
    }

    bool DockerContainer::isDockerAvailable()
    {
        auto [code, out] = runGetOutput("docker --version");
        REACTORMQ_LOG(
            logging::LogLevel::Debug, "[isDockerAvailable] code=%d output=\"%s\" path=\"%s\"", code, out.c_str(), std::getenv("PATH"));

        const bool available = (code == 0);
        REACTORMQ_LOG(logging::LogLevel::Info, "[isDockerAvailable] available=%s", available ? "true" : "false");

        return available;
    }

    bool DockerContainer::waitForRunning(const std::chrono::seconds timeoutSeconds) const
    {
        REACTORMQ_LOG(
            logging::LogLevel::Info, "[waitForRunning] name=\"%s\" timeout=%llds", m_name.c_str(),
            static_cast<long long>(timeoutSeconds.count()));

        const auto deadline = std::chrono::steady_clock::now() + timeoutSeconds;
        while (std::chrono::steady_clock::now() < deadline)
        {
            auto [code, out] = runGetOutput("docker inspect -f \"{{.State.Running}}\" " + m_name);
            REACTORMQ_LOG(logging::LogLevel::Debug, "[waitForRunning] name=\"%s\" code=%d output=\"%s\"", m_name.c_str(), code, out.c_str());

            if (code == 0 && (out == "true" || out == "True"))
            {
                REACTORMQ_LOG(logging::LogLevel::Info, "[waitForRunning] container=\"%s\" is running", m_name.c_str());
                return true;
            }
            std::this_thread::sleep_for(200ms);
        }

        REACTORMQ_LOG(logging::LogLevel::Warn, "[waitForRunning] timeout waiting for container=\"%s\" to become running", m_name.c_str());

        return false;
    }

    bool DockerContainer::waitForPort(const std::chrono::seconds timeoutSeconds) const
    {
        REACTORMQ_LOG(logging::LogLevel::Info, "[waitForPort] hostPort=%u timeout=%llds", m_hostPort,
                      static_cast<long long>(timeoutSeconds.count()));

        const auto deadline = std::chrono::steady_clock::now() + timeoutSeconds;

#if defined(_WIN32)
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

        while (std::chrono::steady_clock::now() < deadline)
        {
#if defined(_WIN32)
            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET)
            {
                std::this_thread::sleep_for(200ms);
                continue;
            }
#else
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                std::this_thread::sleep_for(200ms);
                continue;
            }
#endif

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(m_hostPort);
            if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1)
            {
#if defined(_WIN32)
                closesocket(sock);
#else
                close(sock);
#endif
                std::this_thread::sleep_for(200ms);
                continue;
            }

            const int connectResult = connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

            if (connectResult == 0)
            {
                // TCP connect succeeded. Optionally send MQTT CONNECT and wait for response to confirm broker readiness.
                // Minimal MQTT v5 CONNECT: packetType=0x10, remainingLength=0x1a (26), protocolName="MQTT", level=5, flags=0x02 (cleanStart),
                // keepAlive=30, properties=0, clientId="PROBE_".
                const unsigned char mqttConnect[] = {
                    0x10, 0x14, 0x00, 0x04, 0x4d, 0x51, 0x54, 0x54,
                    0x05, 0x02, 0x00, 0x1e, 0x00, 0x00, 0x07, 0x50,
                    0x52, 0x4f, 0x42, 0x45, 0x5f, 0x00
                };

                const int sendResult = send(sock, reinterpret_cast<const char*>(mqttConnect), sizeof(mqttConnect), 0);
                if (sendResult <= 0)
                {
                    REACTORMQ_LOG(logging::LogLevel::Debug, "[waitForPort] failed to send MQTT CONNECT probe");
#if defined(_WIN32)
                    closesocket(sock);
#else
                    close(sock);
#endif
                    std::this_thread::sleep_for(200ms);
                    continue;
                }

#if defined(_WIN32)
                u_long nonBlocking = 1;
                ioctlsocket(sock, FIONBIO, &nonBlocking);
#else
                int flags = fcntl(sock, F_GETFL, 0);
                fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

                const auto recvDeadline = std::chrono::steady_clock::now() + 500ms;
                bool gotResponse = false;

                while (std::chrono::steady_clock::now() < recvDeadline)
                {
                    unsigned char recvBuf[16];
                    const int recvResult = recv(sock, reinterpret_cast<char*>(recvBuf), sizeof(recvBuf), 0);

                    if (recvResult > 0)
                    {
                        REACTORMQ_LOG(logging::LogLevel::Info, "[waitForPort] broker responded to MQTT CONNECT probe (port=%u)", m_hostPort);
                        gotResponse = true;
                        break;
                    }

                    if (recvResult == 0)
                    {
                        REACTORMQ_LOG(logging::LogLevel::Debug, "[waitForPort] broker closed connection (port=%u)", m_hostPort);
                        break;
                    }

#if defined(_WIN32)
                    const int err = WSAGetLastError();
                    if (err != WSAEWOULDBLOCK)
                    {
                        break;
                    }
#else
                    if (errno != EWOULDBLOCK && errno != EAGAIN)
                    {
                        break;
                    }
#endif
                    std::this_thread::sleep_for(10ms);
                }

#if defined(_WIN32)
                closesocket(sock);
#else
                close(sock);
#endif

                if (gotResponse)
                {
                    REACTORMQ_LOG(logging::LogLevel::Info, "[waitForPort] port %u is ready (MQTT broker responding)", m_hostPort);
                    return true;
                }

                REACTORMQ_LOG(logging::LogLevel::Debug, "[waitForPort] no response from broker on port %u, retrying", m_hostPort);
            }
            else
            {
#if defined(_WIN32)
                closesocket(sock);
#else
                close(sock);
#endif
            }

            std::this_thread::sleep_for(200ms);
        }

        REACTORMQ_LOG(logging::LogLevel::Warn, "[waitForPort] timeout waiting for port %u", m_hostPort);
        return false;
    }

    std::future<bool> DockerContainer::startAsync(std::chrono::seconds timeoutSeconds)
    {
        REACTORMQ_LOG(
            logging::LogLevel::Info,
            "[startAsync] scheduling start for container=\"%s\" image=\"%s\" hostPort=%u containerPort=%u timeout=%llds", m_name.c_str(),
            m_image.c_str(), static_cast<unsigned>(m_hostPort), static_cast<unsigned>(m_containerPort),
            static_cast<long long>(timeoutSeconds.count()));

        return std::async(
            std::launch::async, [this, timeoutSeconds]() -> bool
            {
                REACTORMQ_LOG(logging::LogLevel::Debug, "[startAsync::worker] starting for container=\"%s\"", m_name.c_str());
                const auto deadline = std::chrono::steady_clock::now() + timeoutSeconds;
                if (!isDockerAvailable())
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Error, "[startAsync::worker] Docker is not available, cannot start container=\"%s\"",
                        m_name.c_str());
                    return false;
                }

#if defined(_WIN32)
                const std::string rmCmd = "docker rm -f " + m_name + " >nul 2>&1";
#else
                const std::string rmCmd = "docker rm -f " + m_name + " >/dev/null 2>&1";
#endif
                REACTORMQ_LOG(
                    logging::LogLevel::Debug, "[startAsync::worker] removing any existing container name=\"%s\" command=\"%s\"",
                    m_name.c_str(), rmCmd.c_str());
                run(rmCmd);

                std::ostringstream runCmd;
                runCmd << "docker run --name " << m_name << " -d -p " << m_hostPort << ':' << m_containerPort;
                if (!m_runArgs.empty())
                {
                    runCmd << ' ' << m_runArgs;
                }
                runCmd << ' ' << m_image;

                const auto runCmdStr = runCmd.str();
                REACTORMQ_LOG(
                    logging::LogLevel::Info, "[startAsync::worker] starting container name=\"%s\" image=\"%s\" command=\"%s\"",
                    m_name.c_str(), m_image.c_str(), runCmdStr.c_str());

                if (const int code = run(runCmdStr); code != 0)
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Error, "[startAsync::worker] docker run failed for container=\"%s\" exit_code=%d",
                        m_name.c_str(), code);
                    return false;
                }

                if (!waitForRunning(timeoutSeconds))
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Warn, "[startAsync::worker] container=\"%s\" did not become running within timeout",
                        m_name.c_str());
                    return false;
                }

                if (!waitForPort(timeoutSeconds))
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Warn, "[startAsync::worker] container=\"%s\" port %u did not open within timeout",
                        m_name.c_str(), m_hostPort);
                    return false;
                }

                while (std::chrono::steady_clock::now() < deadline)
                {
                    std::this_thread::sleep_for(200ms);
                }

                m_isRunning.store(true);
                REACTORMQ_LOG(logging::LogLevel::Info, "[startAsync::worker] container=\"%s\" is running", m_name.c_str());

                return true;
            });
    }

    void DockerContainer::stop()
    {
        REACTORMQ_LOG(
            logging::LogLevel::Debug, "[stop] requested stop for container=\"%s\" running=%s", m_name.c_str(),
            m_isRunning.load() ? "true" : "false");

        if (!m_isRunning.load())
        {
            REACTORMQ_LOG(logging::LogLevel::Debug, "[stop] container=\"%s\" not running; nothing to do", m_name.c_str());
            return;
        }

#if defined(_WIN32)
        const std::string stopCmd = "docker stop " + m_name + " >nul 2>&1";
        const std::string rmCmd = "docker rm " + m_name + " >nul 2>&1";
#else
        const std::string stopCmd = "docker stop " + m_name + " >/dev/null 2>&1"; const std::string rmCmd = "docker rm " + m_name +
            " >/dev/null 2>&1";
#endif

        REACTORMQ_LOG(logging::LogLevel::Info, "[stop] stopping container=\"%s\" command=\"%s\"", m_name.c_str(), stopCmd.c_str());
        run(stopCmd);

        REACTORMQ_LOG(logging::LogLevel::Info, "[stop] removing container=\"%s\" command=\"%s\"", m_name.c_str(), rmCmd.c_str());
        run(rmCmd);

        m_isRunning.store(false);
        REACTORMQ_LOG(logging::LogLevel::Info, "[stop] container=\"%s\" marked as not running", m_name.c_str());
    }
} // namespace reactormq::test
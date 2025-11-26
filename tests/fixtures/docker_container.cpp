//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "docker_container.h"
#include "util/logging/logging.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <thread>

#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
#include <WS2tcpip.h>
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267)
#endif

using namespace std::chrono_literals;

namespace
{
#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
    using DockerSocketHandle = SOCKET;
    constexpr DockerSocketHandle InvalidSocket = INVALID_SOCKET;

    void closeSocket(const DockerSocketHandle s)
    {
        closesocket(s);
    }

    bool wouldBlock()
    {
        return WSAGetLastError() == WSAEWOULDBLOCK;
    }

    void setNonBlocking(const DockerSocketHandle s)
    {
        u_long nonBlocking = 1;
        ioctlsocket(s, FIONBIO, &nonBlocking);
    }
#else // REACTORMQ_PLATFORM_WINDOWS_FAMILY
    using DockerSocketHandle = int;
    constexpr DockerSocketHandle InvalidSocket = -1;
    using SockLenArg = socklen_t;
    using IoLenArg = size_t;

    void closeSocket(DockerSocketHandle s)
    {
        close(s);
    }

    bool wouldBlock()
    {
        return errno == EWOULDBLOCK || errno == EAGAIN;
    }

    void setNonBlocking(DockerSocketHandle s)
    {
        const int flags = fcntl(s, F_GETFL, 0);
        fcntl(s, F_SETFL, flags | O_NONBLOCK);
    }
#endif // REACTORMQ_PLATFORM_WINDOWS_FAMILY

    class SocketGuard
    {
    public:
        explicit SocketGuard(const DockerSocketHandle sock)
            : m_sock(sock)
        {
        }

        ~SocketGuard()
        {
            if (m_sock != InvalidSocket)
            {
                closeSocket(m_sock);
            }
        }

        SocketGuard(const SocketGuard&) = delete;
        SocketGuard& operator=(const SocketGuard&) = delete;

        SocketGuard(SocketGuard&& other) noexcept
            : m_sock(other.m_sock)
        {
            other.m_sock = InvalidSocket;
        }

        SocketGuard& operator=(SocketGuard&& other) noexcept
        {
            if (this != &other)
            {
                if (m_sock != InvalidSocket)
                {
                    closeSocket(m_sock);
                }
                m_sock = other.m_sock;
                other.m_sock = InvalidSocket;
            }
            return *this;
        }

        DockerSocketHandle get() const
        {
            return m_sock;
        }

        bool valid() const
        {
            return m_sock != InvalidSocket;
        }

    private:
        DockerSocketHandle m_sock{ InvalidSocket };
    };

    bool fillLoopback(sockaddr_in& addr, const unsigned hostPort)
    {
        addr = sockaddr_in{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(hostPort));
        return inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) == 1;
    }

    bool probeMqttBroker(const DockerSocketHandle sock, const unsigned hostPort)
    {
        constexpr std::array mqttConnect
            = { std::byte{ 0x10 }, std::byte{ 0x14 }, std::byte{ 0x00 }, std::byte{ 0x04 }, std::byte{ 0x4d }, std::byte{ 0x51 },
                std::byte{ 0x54 }, std::byte{ 0x54 }, std::byte{ 0x05 }, std::byte{ 0x02 }, std::byte{ 0x00 }, std::byte{ 0x1e },
                std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x07 }, std::byte{ 0x50 }, std::byte{ 0x52 }, std::byte{ 0x4f },
                std::byte{ 0x42 }, std::byte{ 0x45 }, std::byte{ 0x5f }, std::byte{ 0x00 } };

        const auto* data = static_cast<const char*>(static_cast<const void*>(mqttConnect.data()));
        if (const auto sendResult = send(sock, data, mqttConnect.size(), 0); sendResult <= 0)
        {
            REACTORMQ_LOG(reactormq::logging::LogLevel::Debug, "[waitForPort] failed to send MQTT CONNECT probe");
            return false;
        }

        setNonBlocking(sock);

        const auto recvDeadline = std::chrono::steady_clock::now() + 500ms;
        std::array<std::byte, 16> recvBuf{};

        while (std::chrono::steady_clock::now() < recvDeadline)
        {
            auto* recvPtr = static_cast<char*>(static_cast<void*>(recvBuf.data()));
            const auto recvResult = recv(sock, recvPtr, recvBuf.size(), 0);

            if (recvResult > 0)
            {
                REACTORMQ_LOG(
                    reactormq::logging::LogLevel::Info,
                    "[waitForPort] broker responded to MQTT CONNECT probe (port=%u)",
                    hostPort);
                return true;
            }

            if (recvResult == 0)
            {
                REACTORMQ_LOG(reactormq::logging::LogLevel::Debug, "[waitForPort] broker closed connection (port=%u)", hostPort);
                return false;
            }

            if (!wouldBlock())
            {
                return false;
            }

            std::this_thread::sleep_for(10ms);
        }

        return false;
    }
} // namespace

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

        REACTORMQ_LOG(logging::LogLevel::Debug, "[trim] original_size=%zu trimmed_size=%zu", static_cast<size_t>(originalSize), s.size());

        return s;
    }

    std::pair<int, std::string> DockerContainer::runGetOutput(const std::string& cmd)
    {
        REACTORMQ_LOG(logging::LogLevel::Debug, "[runGetOutput] executing command=\"%s\"", cmd.c_str());

#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
        const std::string full = cmd + " 2>&1";
        FILE* pipe = _popen(full.c_str(), "r");
#else // REACTORMQ_PLATFORM_WINDOWS_FAMILY
        const std::string full = cmd + " 2>&1";
        FILE* pipe = popen(full.c_str(), "r");
#endif // REACTORMQ_PLATFORM_WINDOWS_FAMILY
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

#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
        int code = _pclose(pipe);
#else // REACTORMQ_PLATFORM_WINDOWS_FAMILY
        int code = pclose(pipe);
#endif // REACTORMQ_PLATFORM_WINDOWS_FAMILY

        const auto outStr = trim(output.str());
        REACTORMQ_LOG(
            logging::LogLevel::Debug,
            "[runGetOutput] command=\"%s\" exit_code=%d output=\"%s\"",
            cmd.c_str(),
            code,
            outStr.c_str());

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
        REACTORMQ_LOG(logging::LogLevel::Debug, "[isDockerAvailable] code=%d output=\"%s\"", code, out.c_str());

        const bool available = (code == 0);
        REACTORMQ_LOG(logging::LogLevel::Info, "[isDockerAvailable] available=%s", available ? "true" : "false");

        return available;
    }

    bool DockerContainer::waitForRunning(const std::chrono::seconds timeoutSeconds) const
    {
        REACTORMQ_LOG(logging::LogLevel::Info, "[waitForRunning] name=\"%s\" timeout=%llds", m_name.c_str(), timeoutSeconds.count());

        const auto deadline = std::chrono::steady_clock::now() + timeoutSeconds;
        while (std::chrono::steady_clock::now() < deadline)
        {
            auto [code, out] = runGetOutput("docker inspect -f \"{{.State.Running}}\" " + m_name);
            REACTORMQ_LOG(
                logging::LogLevel::Debug,
                "[waitForRunning] name=\"%s\" code=%d output=\"%s\"",
                m_name.c_str(),
                code,
                out.c_str());

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
        REACTORMQ_LOG(logging::LogLevel::Info, "[waitForPort] hostPort=%u timeout=%llds", m_hostPort, timeoutSeconds.count());

        const auto deadline = std::chrono::steady_clock::now() + timeoutSeconds;

#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

        while (std::chrono::steady_clock::now() < deadline)
        {
#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
            SocketGuard sock(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
#else
            SocketGuard sock(socket(AF_INET, SOCK_STREAM, 0));
#endif
            if (!sock.valid())
            {
                std::this_thread::sleep_for(200ms);
                continue;
            }

            sockaddr_in addr{};
            if (!fillLoopback(addr, m_hostPort))
            {
                std::this_thread::sleep_for(200ms);
                continue;
            }

            const auto* sa = static_cast<const sockaddr*>(static_cast<const void*>(&addr));
            const int connectResult = connect(sock.get(), sa, sizeof(addr));
            if (connectResult != 0)
            {
                std::this_thread::sleep_for(200ms);
                continue;
            }

            if (probeMqttBroker(sock.get(), m_hostPort))
            {
                REACTORMQ_LOG(logging::LogLevel::Info, "[waitForPort] port %u is ready (MQTT broker responding)", m_hostPort);
                return true;
            }

            REACTORMQ_LOG(logging::LogLevel::Debug, "[waitForPort] no response from broker on port %u, retrying", m_hostPort);
            std::this_thread::sleep_for(200ms);
        }

        REACTORMQ_LOG(logging::LogLevel::Warn, "[waitForPort] timeout waiting for port %u", m_hostPort);
        return false;
    }

    std::future<bool> DockerContainer::startAsync(std::chrono::seconds timeoutSeconds)
    {
        REACTORMQ_LOG(
            logging::LogLevel::Info,
            "[startAsync] scheduling start for container=\"%s\" image=\"%s\" hostPort=%u containerPort=%u timeout=%llds",
            m_name.c_str(),
            m_image.c_str(),
            m_hostPort,
            m_containerPort,
            timeoutSeconds.count());

        const auto removeExisting = [this]()
        {
#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
            const std::string rmCmd = "docker rm -f " + m_name + " >nul 2>&1";
#else
            const std::string rmCmd = "docker rm -f " + m_name + " >/dev/null 2>&1";
#endif
            REACTORMQ_LOG(
                logging::LogLevel::Debug,
                "[startAsync::worker] removing any existing container name=\"%s\" command=\"%s\"",
                m_name.c_str(),
                rmCmd.c_str());
            run(rmCmd);
        };

        const auto buildRunCommand = [this]()
        {
            std::ostringstream runCmd;
            runCmd << "docker run --name " << m_name << " -d -p " << m_hostPort << ':' << m_containerPort;
            if (!m_runArgs.empty())
            {
                runCmd << ' ' << m_runArgs;
            }
            runCmd << ' ' << m_image;
            return runCmd.str();
        };

        const auto sleepUntil = [](const auto deadline)
        {
            while (std::chrono::steady_clock::now() < deadline)
            {
                std::this_thread::sleep_for(200ms);
            }
        };

        return std::async(
            std::launch::async,
            [this, timeoutSeconds, removeExisting, buildRunCommand, sleepUntil]
            {
                REACTORMQ_LOG(logging::LogLevel::Debug, "[startAsync::worker] starting for container=\"%s\"", m_name.c_str());
                const auto deadline = std::chrono::steady_clock::now() + timeoutSeconds;
                if (!isDockerAvailable())
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Error,
                        "[startAsync::worker] Docker is not available, cannot start container=\"%s\"",
                        m_name.c_str());
                    return false;
                }

                removeExisting();

                const auto runCmdStr = buildRunCommand();
                REACTORMQ_LOG(
                    logging::LogLevel::Info,
                    "[startAsync::worker] starting container name=\"%s\" image=\"%s\" command=\"%s\"",
                    m_name.c_str(),
                    m_image.c_str(),
                    runCmdStr.c_str());

                if (const int code = run(runCmdStr); code != 0)
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Error,
                        "[startAsync::worker] docker run failed for container=\"%s\" exit_code=%d",
                        m_name.c_str(),
                        code);
                    return false;
                }

                if (!waitForRunning(timeoutSeconds))
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Warn,
                        "[startAsync::worker] container=\"%s\" did not become running within timeout",
                        m_name.c_str());
                    return false;
                }

                if (!waitForPort(timeoutSeconds))
                {
                    REACTORMQ_LOG(
                        logging::LogLevel::Warn,
                        "[startAsync::worker] container=\"%s\" port %u did not open within timeout",
                        m_name.c_str(),
                        m_hostPort);
                    return false;
                }

                sleepUntil(deadline);

                m_isRunning.store(true);
                REACTORMQ_LOG(logging::LogLevel::Info, "[startAsync::worker] container=\"%s\" is running", m_name.c_str());
                return true;
            });
    }

    void DockerContainer::stop()
    {
        REACTORMQ_LOG(
            logging::LogLevel::Debug,
            "[stop] requested stop for container=\"%s\" running=%s",
            m_name.c_str(),
            m_isRunning.load() ? "true" : "false");

        if (!m_isRunning.load())
        {
            REACTORMQ_LOG(logging::LogLevel::Debug, "[stop] container=\"%s\" not running; nothing to do", m_name.c_str());
            return;
        }

#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
        const std::string stopCmd = "docker stop " + m_name + " >nul 2>&1";
        const std::string rmCmd = "docker rm " + m_name + " >nul 2>&1";
#else
        const std::string stopCmd = "docker stop " + m_name + " >/dev/null 2>&1";
        const std::string rmCmd = "docker rm " + m_name + " >/dev/null 2>&1";
#endif

        REACTORMQ_LOG(logging::LogLevel::Info, "[stop] stopping container=\"%s\" command=\"%s\"", m_name.c_str(), stopCmd.c_str());
        run(stopCmd);

        REACTORMQ_LOG(logging::LogLevel::Info, "[stop] removing container=\"%s\" command=\"%s\"", m_name.c_str(), rmCmd.c_str());
        run(rmCmd);

        m_isRunning.store(false);
        REACTORMQ_LOG(logging::LogLevel::Info, "[stop] container=\"%s\" marked as not running", m_name.c_str());
    }
} // namespace reactormq::test

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <atomic>
#include <chrono>
#include <future>
#include <string>

namespace reactormq::test
{
    class DockerContainer
    {
    public:
        DockerContainer(
            std::string name,
            std::string image,
            const std::uint16_t hostPort,
            const std::uint16_t containerPort,
            std::string runArgs = std::string())
            : m_name(std::move(name))
              , m_image(std::move(image))
              , m_hostPort(hostPort)
              , m_containerPort(containerPort)
              , m_runArgs(std::move(runArgs))
        {
        }

        std::future<bool> startAsync(std::chrono::seconds timeoutSeconds);

        void stop();

        [[nodiscard]] bool isRunning() const
        {
            return m_isRunning.load();
        }

        static bool isDockerAvailable();

    private:
        static std::pair<int, std::string> runGetOutput(const std::string& cmd);

        static int run(const std::string& cmd);

        static std::string trim(std::string s);

        [[nodiscard]] bool waitForRunning(std::chrono::seconds timeoutSeconds) const;

        [[nodiscard]] bool waitForPort(std::chrono::seconds timeoutSeconds) const;

        std::string m_name;
        std::string m_image;
        std::uint16_t m_hostPort;
        std::uint16_t m_containerPort;
        std::string m_runArgs;
        std::atomic<bool> m_isRunning{ false };
    };
} // namespace reactormq::test
//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "reactormq/mqtt/connection_settings_builder.h"
#include "util/system/system_info.h"

#include <format>
#include <random>

std::shared_ptr<reactormq::mqtt::ConnectionSettings> reactormq::mqtt::ConnectionSettingsBuilder::build() const
{
    return std::make_shared<ConnectionSettings>(
        m_host,
        m_port,
        m_protocol,
        m_credentialsProvider,
        m_path,
        m_clientId,
        m_maxPacketSize,
        m_maxBufferSize,
        m_maxOutboundQueueBytes,
        m_packetRetryIntervalSeconds,
        m_packetRetryBackoffMultiplier,
        m_maxPacketRetryIntervalSeconds,
        m_socketConnectionTimeoutSeconds,
        m_keepAliveIntervalSeconds,
        m_mqttConnectionTimeoutSeconds,
        m_initialRetryConnectionIntervalSeconds,
        m_maxConnectionRetries,
        m_maxPacketRetries,
        m_shouldVerifyServerCertificate,
        m_sessionExpiryInterval,
        m_autoReconnectEnabled,
        m_autoReconnectInitialDelayMs,
        m_autoReconnectMaxDelayMs,
        m_autoReconnectMultiplier,
        m_strictMode,
        m_enforceMaxPacketSize,
        m_maxInboundPacketsPerTick,
        m_maxPendingCommands,
        m_callbackExecutor,
        m_sslVerifyCallback);
}

std::string reactormq::mqtt::ConnectionSettingsBuilder::generateClientId(const bool withRandomPrefix)
{
    if (!withRandomPrefix)
    {
        return std::format("{}_{}", system::getComputerName(), system::getProcessId());
    }

    std::vector<std::uint8_t> bytes{ 16 };
    std::random_device rd;

    for (auto& b : bytes)
    {
        b = static_cast<std::uint8_t>(rd());
    }

    return std::format(
        "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}_{}{}",
        bytes[0],
        bytes[1],
        bytes[2],
        bytes[3],
        bytes[4],
        bytes[5],
        bytes[6],
        bytes[7],
        bytes[8],
        bytes[9],
        bytes[10],
        bytes[11],
        bytes[12],
        bytes[13],
        bytes[14],
        bytes[15],
        system::getComputerName(),
        system::getProcessId());
}
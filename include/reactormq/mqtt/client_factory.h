//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "reactormq/mqtt/client.h"
#include "reactormq/mqtt/connection_settings.h"

#include <memory>

namespace reactormq::mqtt::client
{
    /**
     * @brief Create an MQTT client instance.
     * @param settings Connection settings for the client.
     * @return Shared pointer to the client interface.
     */
    std::shared_ptr<IClient> createClient(const ConnectionSettingsPtr& settings);
} // namespace reactormq::mqtt::client
#pragma once

#include "reactormq/mqtt/client.h"
#include "reactormq/mqtt/connection_settings.h"

#include <memory>

namespace reactormq::mqtt::client
{
    /**
     * @brief Create an MQTT client instance.
     * @param settings Connection settings for the client.
     * @return Unique pointer to the client implementation.
     */
    std::shared_ptr<IClient> createClient(ConnectionSettingsPtr settings);

} // namespace reactormq::mqtt::client

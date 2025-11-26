//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#include "reactormq/mqtt/client_factory.h"
#include "client_impl.h"

namespace reactormq::mqtt::client
{
    std::shared_ptr<IClient> createClient(const ConnectionSettingsPtr& settings)
    {
        return std::make_shared<ClientImpl>(settings);
    }
} // namespace reactormq::mqtt::client
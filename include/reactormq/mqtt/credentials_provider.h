//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "reactormq/export.h"
#include "reactormq/mqtt/credentials.h"

namespace reactormq::mqtt
{
    /**
     * @brief Source of MQTT credentials and optional enhanced-auth data.
     * Supports static or dynamic credentials and MQTT 5 enhanced authentication flows.
     */
    class REACTORMQ_API ICredentialsProvider
    {
    public:
        virtual ~ICredentialsProvider() = default;

        /**
         * @brief Return credentials for authentication.
         * @return Username and password to use in CONNECT.
         */
        virtual Credentials getCredentials() = 0;

        /**
         * @brief Authentication method for MQTT 5 enhanced auth (AUTH packets).
         * @return Method name such as "SCRAM-SHA-1" when using enhanced auth, or empty if not used.
         */
        virtual std::string getAuthMethod()
        {
            return {};
        }

        /**
         * @brief Initial auth data for client-first methods (sent in CONNECT).
         * @return Bytes to include with CONNECT, or empty when not required.
         */
        virtual std::vector<uint8_t> getInitialAuthData()
        {
            return {};
        }

        /**
         * @brief Handle an AUTH challenge from the server.
         * @param serverData Challenge payload from the server.
         * @return Next client response payload, or empty if no additional data.
         */
        virtual std::vector<uint8_t> onAuthChallenge(const std::vector<uint8_t>& serverData)
        {
            (void)serverData; // Suppress unused parameter warning
            return {};
        }
    };
} // namespace reactormq::mqtt
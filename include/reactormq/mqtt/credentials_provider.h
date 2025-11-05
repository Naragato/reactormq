#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "reactormq/export.h"
#include "reactormq/mqtt/credentials.h"

namespace reactormq::mqtt
{
    /**
     * @brief Interface for MQTT credentials providers.
     * 
     * Allows for different methods of obtaining credentials (static, dynamic, token-based, etc.).
     * Supports MQTT 5 enhanced authentication.
     */
    class REACTORMQ_API ICredentialsProvider
    {
    public:
        virtual ~ICredentialsProvider() = default;

        /**
         * @brief Get the credentials for authentication.
         * @return The credentials (username and password).
         */
        virtual Credentials getCredentials() = 0;

        /**
         * @brief Get the authentication method name for enhanced authentication (MQTT v5, AUTH packets).
         * @return The authentication method name if using enhanced auth (e.g., "SCRAM-SHA-1", "GS2-KRB5").
         *         Empty string if not using enhanced auth.
         */
        virtual std::string getAuthMethod()
        {
            return {};
        }

        /**
         * @brief Get initial authentication data to include in CONNECT when the method requires client-first data.
         * @return Optional initial authentication data.
         *         Empty vector if not using enhanced auth or no initial data required.
         */
        virtual std::vector<uint8_t> getInitialAuthData()
        {
            return {};
        }

        /**
         * @brief Handle a server AUTH challenge and return the next client authentication data.
         * @param serverData The challenge data received from the server.
         * @return The response to the server challenge.
         *         The default implementation returns an empty vector, meaning no additional data.
         */
        virtual std::vector<uint8_t> onAuthChallenge(const std::vector<uint8_t>& serverData)
        {
            (void) serverData; // Suppress unused parameter warning
            return {};
        }
    };
} // namespace reactormq::mqtt

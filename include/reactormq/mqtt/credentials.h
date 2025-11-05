#pragma once

#include <string>
#include "reactormq/export.h"

namespace reactormq::mqtt
{
    /**
     * @brief Represents MQTT credentials.
     * 
     * Contains username and password for MQTT connection authentication.
     */
    struct REACTORMQ_API Credentials final
    {
        /// @brief The username for the connection.
        std::string username;

        /// @brief The password for the connection.
        std::string password;

        /**
         * @brief Default constructor.
         */
        Credentials() = default;

        /**
         * @brief Constructor with username and password.
         * @param user The username.
         * @param pass The password.
         */
        Credentials(std::string user, std::string pass)
            : username(std::move(user))
              , password(std::move(pass))
        {
        }
    };
} // namespace reactormq::mqtt

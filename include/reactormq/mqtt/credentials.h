#pragma once

#include <string>
#include "reactormq/export.h"

namespace reactormq::mqtt
{
    /**
     * @brief Username and password used for MQTT authentication.
     */
    struct REACTORMQ_API Credentials final
    {
        std::string username;
        std::string password;

        Credentials() = default;

        /**
         * @brief Create credentials with a username and password.
         * @param user Username value.
         * @param pass Password value.
         */
        Credentials(std::string user, std::string pass)
            : username(std::move(user))
              , password(std::move(pass))
        {
        }
    };
} // namespace reactormq::mqtt

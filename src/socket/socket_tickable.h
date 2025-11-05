#pragma once

namespace reactormq::socket
{
    /// @brief Interface for socket-like objects that require periodic updates and can be closed.
    struct ISocketTickable
    {
        virtual ~ISocketTickable() = default;

        /**
         * @brief Update the object state. Called periodically.
         */
        virtual void tick() = 0;

        /**
         * @brief Close the object gracefully.
         */
        virtual void close() = 0;
    };
} // namespace reactormq::socket

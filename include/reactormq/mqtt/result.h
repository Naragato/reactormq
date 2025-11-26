//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <memory>

namespace reactormq::mqtt
{
    /**
     * @brief Result type that carries a success flag and an optional value.
     */
    template<typename T>
    class Result
    {
    public:
        /**
         * @brief Construct with success flag and no value.
         * @param success True if the operation succeeded.
         */
        explicit Result(const bool success)
            : m_success(success)
        {
        }

        /**
         * @brief Construct with success flag and a value.
         * @param success True if the operation succeeded.
         * @param value Value to store (moved).
         */
        Result(const bool success, T&& value)
            : m_value(std::make_shared<T>(std::move(value)))
            , m_success(success)
        {
        }

        /**
         * @brief Create a successful result with a value.
         * @param value Value to store (moved).
         * @return Success result with the given value.
         */
        static Result success(T&& value)
        {
            return Result(true, std::move(value));
        }

        /**
         * @brief Create a successful result with no value.
         * @return Success result.
         */
        static Result success()
        {
            return Result(true);
        }

        /**
         * @brief Create a failed result.
         * @param reason Human-readable reason (not stored).
         * @return Failure result.
         */
        static Result failure(const char* reason)
        {
            (void)reason;
            return Result(false);
        }

        /**
         * @brief Get the stored value if present.
         * @return Shared pointer to the value, or null when no value is stored.
         */
        [[nodiscard]] std::shared_ptr<T> getResult() const
        {
            return m_value;
        }

        /**
         * @brief Whether the associated operation succeeded.
         * @return True on success.
         */
        [[nodiscard]] bool hasSucceeded() const
        {
            return m_success;
        }

        /**
         * @brief Alias for hasSucceeded(); used by tests.
         */
        [[nodiscard]] bool isSuccess() const
        {
            return hasSucceeded();
        }

    private:
        std::shared_ptr<T> m_value;
        bool m_success{ false };
    };

    template<>
    class Result<void>
    {
    public:
        /**
         * @brief Construct with success flag.
         * @param success True if the operation succeeded.
         */
        explicit Result(const bool success)
            : m_success(success)
        {
        }

        /**
         * @brief Create a successful result.
         * @return Success result.
         */
        static Result success()
        {
            return Result(true);
        }

        /**
         * @brief Create a failed result.
         * @param reason Human-readable reason (not stored).
         * @return Failure result.
         */
        static Result failure(const char* reason)
        {
            (void)reason; // Not stored in current simple design
            return Result(false);
        }

        /**
         * @brief Whether the associated operation succeeded.
         * @return True on success.
         */
        [[nodiscard]] bool hasSucceeded() const
        {
            return m_success;
        }

        /**
         * @brief Alias for hasSucceeded(); used by tests.
         */
        [[nodiscard]] bool isSuccess() const
        {
            return hasSucceeded();
        }

    private:
        bool m_success{ false };
    };
} // namespace reactormq::mqtt
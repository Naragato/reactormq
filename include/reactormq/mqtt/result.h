#pragma once

#include <memory>

namespace reactormq::mqtt
{
    /**
     * @brief A simple result type that carries success flag and optional value.
     */
    template <typename T>
    class Result
    {
    public:
        /**
         * @brief Construct a Result with success flag and no value.
         * @param success True if the operation succeeded.
         */
        explicit Result(const bool success)
            : m_success(success)
        {
        }

        /**
         * @brief Construct a Result with success flag and a value.
         * @param success True if the operation succeeded.
         * @param value The value to carry (moved into internal storage).
         */
        Result(const bool success, T&& value)
            : m_value(std::make_shared<T>(std::move(value)))
            , m_success(success)
        {
        }

        /**
         * @brief Create a successful result with a value.
         * @param value The value to carry (moved into internal storage).
         * @return A Result indicating success with the given value.
         */
        static Result success(T&& value)
        {
            return Result(true, std::move(value));
        }

        /**
         * @brief Create a successful result with no value.
         * @return A Result indicating success with no value.
         */
        static Result success()
        {
            return Result(true);
        }

        /**
         * @brief Create a failed result.
         * @param reason Human-readable reason for failure (currently not stored).
         * @return A Result indicating failure.
         */
        static Result failure(const char* reason)
        {
            (void)reason;
            return Result(false);
        }

        /**
         * @brief Get the carried value if present.
         * @return Shared pointer to the value, or null if no value is present.
         */
        std::shared_ptr<T> getResult() const
        {
            return m_value;
        }

        /**
         * @brief Whether the associated operation succeeded.
         * @return True if the operation succeeded.
         */
        [[nodiscard]] bool hasSucceeded() const
        {
            return m_success;
        }

        /**
         * @brief Backward/alternate API used in tests: alias for hasSucceeded().
         */
        [[nodiscard]] bool isSuccess() const
        {
            return hasSucceeded();
        }

    private:
        std::shared_ptr<T> m_value;
        bool m_success { false };
    };

    template <>
    class Result<void>
    {
    public:
        /**
         * @brief Construct a Result<void> with success flag.
         * @param success True if the operation succeeded.
         */
        explicit Result(const bool success)
            : m_success(success)
        {
        }

        /**
         * @brief Create a successful result.
         * @return A Result<void> indicating success.
         */
        static Result success()
        {
            return Result(true);
        }

        /**
         * @brief Create a failed result.
         * @param reason Human-readable reason for failure (currently not stored).
         * @return A Result<void> indicating failure.
         */
        static Result failure(const char* reason)
        {
            (void)reason; // Not stored in current simple design
            return Result(false);
        }

        /**
         * @brief Whether the associated operation succeeded.
         * @return True if the operation succeeded.
         */
        [[nodiscard]] bool hasSucceeded() const
        {
            return m_success;
        }

        /**
         * @brief Backward/alternate API used in tests: alias for hasSucceeded().
         */
        [[nodiscard]] bool isSuccess() const
        {
            return hasSucceeded();
        }

    private:
        bool m_success { false };
    };
}

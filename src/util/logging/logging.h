//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

// Inspired by Roku's rostd::printx approach (traits-based argument forwarding for printf-family calls).

#include "log_level.h"
#include "util/logging/registry.h"

#include <array>
#include <chrono>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

namespace reactormq::logging
{
    std::string toIso8601ms(const std::chrono::system_clock::time_point& tp);
    std::string threadIdStr(const std::jthread::id& id);
    std::string toHex(std::span<const std::uint8_t> data);
    const char* levelToString(LogLevel lvl);

#if REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK
    inline ELogVerbosity::Type ToUeVerbosityForMacro(const LogLevel Level)
    {
        switch (Level)
        {
        case LogLevel::Trace:
            return ELogVerbosity::VeryVerbose;
        case LogLevel::Debug:
            return ELogVerbosity::Verbose;
        case LogLevel::Info:
            return ELogVerbosity::Log;
        case LogLevel::Warn:
            return ELogVerbosity::Warning;
        case LogLevel::Error:
            return ELogVerbosity::Error;
        case LogLevel::Critical:
            return ELogVerbosity::Fatal;
        case LogLevel::Off:
        default:
            return ELogVerbosity::NoLogging;
        }
    }

    inline bool ShouldLogToUeCategory(const LogLevel Level)
    {
        switch (ToUeVerbosityForMacro(Level))
        {
        case ELogVerbosity::Fatal:
            return UE_LOG_ACTIVE(LogReactorMQ, Fatal);
        case ELogVerbosity::Error:
            return UE_LOG_ACTIVE(LogReactorMQ, Error);
        case ELogVerbosity::Warning:
            return UE_LOG_ACTIVE(LogReactorMQ, Warning);
        case ELogVerbosity::Display:
            return UE_LOG_ACTIVE(LogReactorMQ, Display);
        case ELogVerbosity::Log:
            return UE_LOG_ACTIVE(LogReactorMQ, Log);
        case ELogVerbosity::Verbose:
            return UE_LOG_ACTIVE(LogReactorMQ, Verbose);
        case ELogVerbosity::VeryVerbose:
            return UE_LOG_ACTIVE(LogReactorMQ, VeryVerbose);
        case ELogVerbosity::NumVerbosity:
            return UE_LOG_ACTIVE(LogReactorMQ, NumVerbosity);
        case ELogVerbosity::VerbosityMask:
            return UE_LOG_ACTIVE(LogReactorMQ, VerbosityMask);
        case ELogVerbosity::SetColor:
            return UE_LOG_ACTIVE(LogReactorMQ, SetColor);
        case ELogVerbosity::BreakOnLog:
            return UE_LOG_ACTIVE(LogReactorMQ, BreakOnLog);
        case ELogVerbosity::NoLogging:
        default:
            return false;
        }
    }
#endif // REACTORMQ_WITH_UE5 && REACTORMQ_WITH_UE_LOG_SINK

    namespace detail
    {
        constexpr size_t kSmallBufferSize = 256;

        template<class T>
        using RemoveCvRef = std::remove_cv_t<std::remove_reference_t<T>>;

        /**
         * @brief Status of a formatting operation
         */
        enum class FormatStatus : std::uint8_t
        {
            Fit,
            TooBig,
            Error
        };

        /**
         * @brief Result of a format attempt with explicit status
         */
        struct FormatView
        {
            FormatStatus status = FormatStatus::Error;
            std::string_view view{};
        };

        template<class T>
        concept HasCharCString = requires(const T& value) {
            { value.c_str() } -> std::convertible_to<const char*>;
        };

        template<class T, class = void>
        struct PrintfTraits
        {
            static auto forwardArgs(const T& value)
            {
                return std::forward_as_tuple(value);
            }
        };

        template<class T>
        struct PrintfTraits<T, std::enable_if_t<HasCharCString<T>>>
        {
            static auto forwardArgs(const T& value)
            {
                return std::forward_as_tuple(value.c_str());
            }
        };

        template<class... Args>
        auto forwardAllArgs(Args&&... args)
        {
            return std::tuple_cat(PrintfTraits<RemoveCvRef<Args>>::forwardArgs(args)...);
        }

        template<class Tuple>
        int snprintfTo(char* outBuffer, const size_t capacity, const char* formatCstr, const Tuple& argsTuple) noexcept
        {
            return std::apply(
                [&](const auto&... xs)
                {
                    return std::snprintf(outBuffer, capacity, formatCstr, xs...);
                },
                argsTuple);
        }

        template<class Tuple>
        std::string formatSnprintfWithTuple(const char* formatCstr, const Tuple& argsTuple)
        {
            if (formatCstr == nullptr)
            {
                return {};
            }

            std::array<char, kSmallBufferSize> small{};
            int count = snprintfTo(small.data(), small.size(), formatCstr, argsTuple);

#if defined(_MSC_VER)
            if (count < 0)
            {
                count = std::apply(
                    [&](const auto&... xs)
                    {
                        return std::snprintf(nullptr, 0, formatCstr, xs...);
                    },
                    argsTuple);
            }
#endif

            if (count < 0)
            {
                return {};
            }

            if (static_cast<size_t>(count) < small.size())
            {
                return { small.data(), static_cast<size_t>(count) };
            }

            std::string out(static_cast<size_t>(count) + 1, '\0');
            const int count2 = snprintfTo(out.data(), out.size(), formatCstr, argsTuple);
            if (count2 < 0)
            {
                return {};
            }

            out.resize(static_cast<size_t>(count2));
            return out;
        }

        inline std::string formatNoArgs(const char* formatCstr)
        {
            return formatCstr != nullptr ? std::string{ formatCstr } : std::string{};
        }

        inline std::string formatNoArgs(const std::string_view formatView)
        {
            return std::string{ formatView };
        }

        /**
         * @brief Lazy formatter that captures format string and arguments,
         * formatting on demand. Tries caller-provided buffer first, falls back to heap.
         */
        template<class... Args>
        class LazyFormat
        {
        public:
            template<class... Ts>
            constexpr explicit LazyFormat(const char* fmt, Ts&&... args) noexcept
                : m_fmt(fmt)
                , m_args(std::forward<Ts>(args)...)
            {
            }

            [[nodiscard]] FormatView tryFormatTo(std::span<char> buffer) const noexcept
            {
                if (!m_fmt || buffer.empty())
                {
                    return { FormatStatus::Error, {} };
                }

                const auto argsTuple = std::apply(
                    []<class... At>(At&&... at)
                    {
                        return forwardAllArgs(std::forward<At>(at)...);
                    },
                    m_args);

                const int count = snprintfTo(buffer.data(), buffer.size(), m_fmt, argsTuple);
                if (count < 0)
                {
                    return { FormatStatus::Error, {} };
                }

                if (static_cast<size_t>(count) < buffer.size())
                {
                    return { FormatStatus::Fit, std::string_view{ buffer.data(), static_cast<size_t>(count) } };
                }

                return { FormatStatus::TooBig, {} };
            }

            [[nodiscard]] std::string formatToString() const
            {
                const auto argsTuple = std::apply(
                    []<class... Fs>(Fs&&... fs)
                    {
                        return forwardAllArgs(std::forward<Fs>(fs)...);
                    },
                    m_args);
                return formatSnprintfWithTuple(m_fmt, argsTuple);
            }

        private:
            const char* m_fmt = nullptr;
            std::tuple<Args...> m_args;
        };

        /**
         * @brief LazyFormat specialization for literal-only case (no arguments).
         */
        template<>
        class LazyFormat<>
        {
        public:
            constexpr explicit LazyFormat(const char* fmt) noexcept
                : m_fmt(fmt)
            {
            }

            [[nodiscard]] FormatView tryFormatTo(std::span<char>) const noexcept
            {
                return m_fmt ? FormatView{ FormatStatus::Fit, std::string_view{ m_fmt } } : FormatView{ FormatStatus::Error, {} };
            }

            [[nodiscard]] std::string formatToString() const
            {
                return m_fmt ? std::string{ m_fmt } : std::string{};
            }

        private:
            const char* m_fmt = nullptr;
        };

        template<class... Args>
        LazyFormat(const char*, Args&&...) -> LazyFormat<Args...>;

        /**
         * @brief Owning small-string buffer: stack fast path and heap fallback.
         * Ensures std::string_view passed to sinks always points at stable storage.
         */
        class SmallLogString
        {
        public:
            [[nodiscard]] std::span<char> inlineBuffer() noexcept
            {
                return m_inline;
            }

            void setInlineSize(const size_t n) noexcept
            {
                m_isHeap = false;
                m_inlineSize = n;
            }

            void setHeap(std::string s)
            {
                m_heap = std::move(s);
                m_isHeap = true;
            }

            [[nodiscard]] std::string_view view() const noexcept
            {
                if (m_isHeap)
                {
                    return std::string_view{ m_heap };
                }
                return std::string_view{ m_inline.data(), m_inlineSize };
            }

        private:
            std::array<char, kSmallBufferSize> m_inline{};
            size_t m_inlineSize = 0;
            bool m_isHeap = false;
            std::string m_heap{};
        };
    } // namespace detail

    template<class... Args>
    [[nodiscard]] std::string format(const char* formatCstr, Args&&... args)
    {
        if constexpr (sizeof...(Args) == 0)
        {
            return detail::formatNoArgs(formatCstr);
        }
        else
        {
            const auto argsTuple = detail::forwardAllArgs(std::forward<Args>(args)...);
            return detail::formatSnprintfWithTuple(formatCstr, argsTuple);
        }
    }

    template<class... Args>
    [[nodiscard]] std::string format(const std::string_view formatView, Args&&... args)
    {
        if constexpr (sizeof...(Args) == 0)
        {
            return detail::formatNoArgs(formatView);
        }
        else
        {
            const std::string owned(formatView);
            const auto argsTuple = detail::forwardAllArgs(std::forward<Args>(args)...);
            return detail::formatSnprintfWithTuple(owned.c_str(), argsTuple);
        }
    }

    template<class... Args>
    [[nodiscard]] std::string formatHexMessage(const void* data, const size_t lengthBytes, const char* fmt, Args&&... args)
    {
        std::string message = logging::format(fmt, std::forward<Args>(args)...);
        message += " | ";
        message += toHex(std::span{ static_cast<const std::uint8_t*>(data), lengthBytes });
        return message;
    }

    template<class... Args>
    void Registry::log(const LogLevel level, const std::source_location& location, const detail::LazyFormat<Args...>& lazyMsg) const
    {
        detail::SmallLogString owned;

        const detail::FormatView r = lazyMsg.tryFormatTo(owned.inlineBuffer());

        if (r.status == detail::FormatStatus::Fit)
        {
            if (r.view.size() < detail::kSmallBufferSize)
            {
                std::memcpy(owned.inlineBuffer().data(), r.view.data(), r.view.size());
                owned.setInlineSize(r.view.size());
            }
            else
            {
                owned.setHeap(std::string{ r.view });
            }

            log(level, location, owned.view());
            return;
        }

        if (r.status == detail::FormatStatus::TooBig)
        {
            owned.setHeap(lazyMsg.formatToString());
            log(level, location, owned.view());
            return;
        }

        log(level, location, "[log format error]");
    }

#define REACTORMQ_LOG(level, fmt, ...)                                                                                                     \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (::reactormq::logging::Registry::instance().shouldLog((level)))                                                                 \
        {                                                                                                                                  \
            const auto lazyMsg = ::reactormq::logging::detail::LazyFormat((fmt)__VA_OPT__(, ) __VA_ARGS__);                                \
            ::reactormq::logging::Registry::instance().log((level), std::source_location::current(), lazyMsg);                             \
        }                                                                                                                                  \
    } while (0)

#define REACTORMQ_LOG_HEX(level, data, length, fmt, ...)                                                                                   \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (::reactormq::logging::Registry::instance().shouldLog((level)))                                                                 \
        {                                                                                                                                  \
            ::reactormq::logging::Registry::instance().log(                                                                                \
                (level),                                                                                                                   \
                std::source_location::current(),                                                                                           \
                ::reactormq::logging::formatHexMessage((data), static_cast<size_t>(length), (fmt)__VA_OPT__(, ) __VA_ARGS__));             \
        }                                                                                                                                  \
    } while (0)
} // namespace reactormq::logging
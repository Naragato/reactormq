//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

namespace reactormq::mqtt
{
    struct Message;
    struct SubscribeResult;
    struct UnsubscribeResult;

    enum class DisconnectPolicy
    {
        OnDestruct,
        Manual
    };

    /**
     * @brief Handle object for a registered delegate callback.
     *
     * Owns the connection to a MulticastDelegate. Depending on policy,
     * the callback disconnects on destruction or only when disconnect() is called.
     */
    class DelegateHandle
    {
    public:
        DelegateHandle() = default;

        /**
         * @brief Construct a handle bound to a specific delegate slot.
         * @param owner Weak reference guarding the delegate lifetime.
         * @param token Weak reference guarding this slot.
         * @param disconnect Callable that removes the slot from the delegate.
         * @param policy Disconnect behavior on destruction.
         */
        DelegateHandle(
            std::weak_ptr<void> owner, std::weak_ptr<void> token, std::function<void()> disconnect, const DisconnectPolicy policy)
            : m_owner(std::move(owner))
            , m_token(std::move(token))
            , m_disconnect(std::move(disconnect))
            , m_policy(policy)
        {
        }

        ~DelegateHandle()
        {
            if (m_policy == DisconnectPolicy::OnDestruct)
            {
                disconnect();
            }
        }

        DelegateHandle(const DelegateHandle&) = delete;

        DelegateHandle& operator=(const DelegateHandle&) = delete;

        DelegateHandle(DelegateHandle&& other) noexcept
            : m_owner(std::move(other.m_owner))
            , m_token(std::move(other.m_token))
            , m_disconnect(std::move(other.m_disconnect))
            , m_policy(other.m_policy)
        {
            other.m_owner.reset();
            other.m_token.reset();
            other.m_disconnect = nullptr;
            other.m_policy = DisconnectPolicy::Manual;
        }

        DelegateHandle& operator=(DelegateHandle&& other) noexcept
        {
            if (this != &other)
            {
                if (m_disconnect && !m_owner.expired())
                {
                    m_disconnect();
                }

                m_owner = std::move(other.m_owner);
                m_token = std::move(other.m_token);
                m_disconnect = std::move(other.m_disconnect);
                m_policy = other.m_policy;

                other.m_owner.reset();
                other.m_token.reset();
                other.m_disconnect = nullptr;
                other.m_policy = DisconnectPolicy::Manual;
            }
            return *this;
        }

        /**
         * @brief Disconnect this callback from the delegate.
         */
        void disconnect() noexcept
        {
            if (!m_disconnect)
            {
                m_owner.reset();
                m_token.reset();
                return;
            }

            if (!m_owner.expired())
            {
                m_disconnect();
            }

            m_disconnect = nullptr;
            m_owner.reset();
            m_token.reset();
        }

        /**
         * @brief Check whether the underlying delegate and slot are still active.
         * @return True when disconnectable; false after the delegate or slot expired.
         */
        [[nodiscard]] bool isValid() const noexcept
        {
            return m_disconnect && !m_owner.expired() && !m_token.expired();
        }

    private:
        std::weak_ptr<void> m_owner;
        std::weak_ptr<void> m_token;
        std::function<void()> m_disconnect;
        DisconnectPolicy m_policy{ DisconnectPolicy::Manual };
    };

    namespace delegate
    {
        template<class T>
        concept SharedFromThis = std::derived_from<T, std::enable_shared_from_this<T>>;

        template<class F, class R, class... Args>
        concept DelegateCallable = std::is_invocable_r_v<R, F&, Args...> && !std::is_member_function_pointer_v<F>;
    } // namespace delegate

    /**
     * @brief Thread-safe multicast delegate for callbacks of a given signature.
     *
     * Supports adding free functions, lambdas, and member functions. Broadcast
     * invokes live callbacks in registration order and prunes expired ones.
     * @tparam Signature Function signature R(Args...).
     */
    template<class Signature>
    class MulticastDelegate;

    template<class R, class... Args>
    class MulticastDelegate<R(Args...)>
    {
        using OptionalR = std::conditional_t<std::is_void_v<R>, std::monostate, R>;

        struct SlotBase
        {
            size_t id{};
            std::function<std::optional<OptionalR>(Args...)> call;
            std::function<bool()> expired;
            std::shared_ptr<void> keeper;
            std::shared_ptr<void> token;
        };

    public:
        MulticastDelegate() = default;

        /**
         * @brief Add a free function or lambda.
         * @tparam F Callable type compatible with R(Args...).
         * @param f Callable to register.
         * @return Handle that can disconnect this callback.
         */
        template<class F>
        DelegateHandle add(F&& f)
        {
            using Fn = std::decay_t<F>;
            static_assert(std::is_invocable_r_v<R, Fn&, Args...>, "Callable must be invocable with Args... and return R");

            auto sp = std::make_shared<Fn>(std::forward<F>(f));
            auto wsp = std::weak_ptr<Fn>(sp);

            SlotBase slot;
            slot.id = m_nextId++;
            slot.token = std::make_shared<std::atomic<bool>>(true);
            std::weak_ptr<void> wtoken = slot.token;
            slot.keeper = sp;
            slot.expired = [wsp]
            {
                return wsp.expired();
            };

            if constexpr (std::is_void_v<R>)
            {
                slot.call = [wsp](Args... args) -> std::optional<OptionalR>
                {
                    if (auto fShared = wsp.lock())
                    {
                        (*fShared)(args...);
                        return std::optional<OptionalR>{ std::monostate{} };
                    }
                    return std::nullopt;
                };
            }
            else
            {
                slot.call = [wsp](Args... args) -> std::optional<OptionalR>
                {
                    if (auto fShared = wsp.lock())
                    {
                        return std::optional<OptionalR>((*fShared)(args...));
                    }
                    return std::nullopt;
                };
            }

            auto id = addSlot(std::move(slot));
            return DelegateHandle(
                m_self,
                wtoken,
                [this, id]
                {
                    this->remove(id);
                },
                DisconnectPolicy::Manual);
        }

        /**
         * @brief Add a callable stored in shared_ptr.
         * @tparam Callable Callable type compatible with R(Args...).
         * @param cb A shared pointer to the callable.
         * @return Handle that can disconnect this callback.
         */
        template<class Callable>
        DelegateHandle addCallable(const std::shared_ptr<Callable>& cb)
        {
            static_assert(std::is_invocable_r_v<R, Callable&, Args...>, "Callable must be invocable with Args... and return R");
            auto wcb = std::weak_ptr<Callable>(cb);

            SlotBase slot;
            slot.id = m_nextId++;
            slot.token = std::make_shared<std::atomic<bool>>(true);
            std::weak_ptr<void> wtoken = slot.token;
            slot.expired = [wcb]
            {
                return wcb.expired();
            };

            if constexpr (std::is_void_v<R>)
            {
                slot.call = [wcb](Args... args) -> std::optional<OptionalR>
                {
                    if (auto sp = wcb.lock())
                    {
                        (*sp)(args...);
                        return std::optional<OptionalR>{ std::monostate{} };
                    }
                    return std::nullopt;
                };
            }
            else
            {
                slot.call = [wcb](Args... args) -> std::optional<OptionalR>
                {
                    if (auto sp = wcb.lock())
                    {
                        return std::optional<OptionalR>((*sp)(args...));
                    }
                    return std::nullopt;
                };
            }

            auto handle = addSlot(std::move(slot));
            return DelegateHandle(
                m_self,
                wtoken,
                [this, id = handle]
                {
                    this->remove(id);
                },
                DisconnectPolicy::Manual);
        }

        /**
         * @brief Add a lambda callback tied to object lifetime (for enable_shared_from_this).
         * @tparam T Object type (must inherit from enable_shared_from_this<T>).
         * @tparam F Callable type with signature compatible with R(Args...).
         * @param obj Raw pointer to object; lifetime is managed via shared_from_this().
         * @param lambda Lambda or callable to register (signature must match R(Args...)).
         * @return Handle that can disconnect this callback.
         *
         * This overload allows you to write:
         *   delegate.addMember(this, [this](Args... args) { ... });
         *
         * The lambda's lifetime is tied to the object's shared_ptr, so it will
         * automatically disconnect when the object is destroyed.
         */
        template<class T, class F>
            requires(delegate::SharedFromThis<T> && delegate::DelegateCallable<F, R, Args...>)
        DelegateHandle add(T* obj, F&& lambda)
        {
            static_assert(
                std::is_base_of_v<std::enable_shared_from_this<T>, T>,
                "T must inherit from std::enable_shared_from_this<T> to use raw pointer overload");

            static_assert(std::is_invocable_r_v<R, F&, Args...>, "Lambda must be callable with Args... and return R");

            std::shared_ptr<T> sharedObj = obj->shared_from_this();

            using Fn = std::decay_t<F>;
            auto lambdaPtr = std::make_shared<Fn>(std::forward<F>(lambda));
            auto wlambda = std::weak_ptr<Fn>(lambdaPtr);
            auto wobj = std::weak_ptr<T>(sharedObj);

            SlotBase slot;
            slot.id = m_nextId++;
            slot.token = std::make_shared<std::atomic<bool>>(true);
            std::weak_ptr<void> wtoken = slot.token;
            slot.keeper = lambdaPtr;

            slot.expired = [wobj, wlambda]
            {
                return wobj.expired() || wlambda.expired();
            };

            if constexpr (std::is_void_v<R>)
            {
                slot.call = [wobj, wlambda](Args... args) -> std::optional<OptionalR>
                {
                    if (auto objPtr = wobj.lock())
                    {
                        if (auto fnPtr = wlambda.lock())
                        {
                            (*fnPtr)(args...);
                            return std::optional<OptionalR>{ std::monostate{} };
                        }
                    }
                    return std::nullopt;
                };
            }
            else
            {
                slot.call = [wobj, wlambda](Args... args) -> std::optional<OptionalR>
                {
                    if (auto objPtr = wobj.lock())
                    {
                        if (auto fnPtr = wlambda.lock())
                        {
                            return std::optional<OptionalR>((*fnPtr)(args...));
                        }
                    }
                    return std::nullopt;
                };
            }

            auto handle = addSlot(std::move(slot));
            return DelegateHandle(
                m_self,
                wtoken,
                [this, id = handle]
                {
                    this->remove(id);
                },
                DisconnectPolicy::Manual);
        }

        /**
         * @brief Add a member function callback using this pointer (for enable_shared_from_this).
         * @tparam T Object type (must inherit from enable_shared_from_this<T>).
         * @tparam MemFn Member function or invocable taking (T&, Args...).
         * @param obj Raw pointer to object; must be convertible to shared_ptr via shared_from_this().
         * @param mf Member function pointer or compatible invocable.
         * @return Handle that can disconnect this callback.
         */
        template<class T, class MemFn>
        DelegateHandle addMember(T* obj, MemFn mf)
        {
            static_assert(
                std::is_base_of_v<std::enable_shared_from_this<T>, T>,
                "T must inherit from std::enable_shared_from_this<T> to use raw pointer overload");

            static_assert(
                std::is_invocable_r_v<R, MemFn, T&, Args...> || std::is_invocable_r_v<R, MemFn, T*, Args...>,
                "Member function must be callable with (T&, Args...) or (T*, Args...)");

            std::shared_ptr<T> sharedObj = obj->shared_from_this();

            return addMember(sharedObj, mf);
        }

        /**
         * @brief Add a member function callback.
         * @tparam T Object type.
         * @tparam MemFn Member function or invocable taking (T&, Args...).
         * @param obj Shared pointer to the object; controls lifetime of the slot.
         * @param mf Member function pointer or compatible invocable.
         * @return Handle that can disconnect this callback.
         */
        template<class T, class MemFn>
        DelegateHandle addMember(const std::shared_ptr<T>& obj, MemFn mf)
        {
            static_assert(
                std::is_invocable_r_v<R, MemFn, T&, Args...> || std::is_invocable_r_v<R, MemFn, T*, Args...>,
                "Member function must be callable with (T&, Args...) or (T*, Args...)");

            auto wobj = std::weak_ptr<T>(obj);

            SlotBase slot;
            slot.id = m_nextId++;
            slot.token = std::make_shared<std::atomic<bool>>(true);
            std::weak_ptr<void> wtoken = slot.token;
            slot.expired = [wobj]
            {
                return wobj.expired();
            };

            if constexpr (std::is_void_v<R>)
            {
                slot.call = [wobj, mf](Args... args) -> std::optional<OptionalR>
                {
                    if (auto sp = wobj.lock())
                    {
                        if constexpr (std::is_member_function_pointer_v<MemFn>)
                        {
                            (sp.get()->*mf)(args...);
                        }
                        else
                        {
                            mf(*sp, args...);
                        }
                        return std::optional<OptionalR>{ std::monostate{} };
                    }
                    return std::nullopt;
                };
            }
            else
            {
                slot.call = [wobj, mf](Args... args) -> std::optional<OptionalR>
                {
                    if (auto sp = wobj.lock())
                    {
                        if constexpr (std::is_member_function_pointer_v<MemFn>)
                        {
                            return std::optional<OptionalR>((sp.get()->*mf)(args...));
                        }
                        else
                        {
                            return std::optional<OptionalR>(mf(*sp, args...));
                        }
                    }
                    return std::nullopt;
                };
            }

            auto handle = addSlot(std::move(slot));
            return DelegateHandle(
                m_self,
                wtoken,
                [this, id = handle]
                {
                    this->remove(id);
                },
                DisconnectPolicy::Manual);
        }

        /**
         * @brief Alias of addCallable for shared_ptr callables.
         * @tparam Callable Callable type.
         * @param cb A shared pointer to the callable.
         * @return Handle that can disconnect this callback.
         */
        template<class Callable>
        DelegateHandle add(const std::shared_ptr<Callable>& cb)
        {
            return addCallable(cb);
        }

        /**
         * @brief Remove a callback by its internal id.
         * @param id Identifier returned when the slot was added.
         */
        void remove(size_t id) noexcept
        {
            std::scoped_lock lock(m_mutex);
            for (auto it = m_slots.begin(); it != m_slots.end(); ++it)
            {
                if (it->id == id)
                {
                    if (it->token)
                    {
                        *std::static_pointer_cast<std::atomic<bool>>(it->token) = false;
                        it->token.reset();
                    }
                    m_slots.erase(it);
                    break;
                }
            }
        }

        /**
         * @brief Remove all registered callbacks.
         */
        void clear() noexcept
        {
            std::scoped_lock lock(m_mutex);
            for (auto& s : m_slots)
            {
                if (s.token)
                {
                    *std::static_pointer_cast<std::atomic<bool>>(s.token) = false;
                    s.token.reset();
                }
            }
            m_slots.clear();
        }

        /**
         * @brief Invoke all current callbacks.
         *
         * For R=void, returns void. For non-void, returns results from live callbacks
         * in registration order.
         * @param args Arguments forwarded to each callback.
         * @return void or std::vector<R> depending on R.
         */
        std::conditional_t<std::is_void_v<R>, void, std::vector<R>> broadcast(Args... args)
        {
            std::vector<SlotBase> snapshot;
            {
                std::scoped_lock lock(m_mutex);
                snapshot.reserve(m_slots.size());
                for (auto it = m_slots.begin(); it != m_slots.end();)
                {
                    if (it->expired())
                    {
                        it = m_slots.erase(it);
                    }
                    else
                    {
                        snapshot.push_back(*it);
                        ++it;
                    }
                }
            }

            if constexpr (std::is_void_v<R>)
            {
                for (auto& s : snapshot)
                {
                    if (s.token && *std::static_pointer_cast<std::atomic<bool>>(s.token))
                    {
                        auto res = s.call(args...);
                        (void)res;
                    }
                }
                return;
            }
            else
            {
                std::vector<R> out;
                out.reserve(snapshot.size());
                for (auto& s : snapshot)
                {
                    if (s.token && *std::static_pointer_cast<std::atomic<bool>>(s.token))
                    {
                        if (auto res = s.call(args...))
                        {
                            out.push_back(std::move(*res));
                        }
                    }
                }
                return out;
            }
        }

        /**
         * @brief Approximate count of currently registered callbacks.
         * @return Number of slots; stale entries are pruned on add/remove/broadcast.
         */
        size_t getSize() const noexcept
        {
            std::scoped_lock lock(m_mutex);
            return m_slots.size();
        }

    private:
        size_t addSlot(SlotBase&& slot)
        {
            std::scoped_lock lock(m_mutex);
            m_slots.push_back(std::move(slot));
            return m_slots.back().id;
        }

        mutable std::mutex m_mutex;
        std::vector<SlotBase> m_slots;
        std::atomic<size_t> m_nextId{ 1 };
        std::shared_ptr<void> m_self{ std::make_shared<int>(0) };
    };

    /**
     * @brief Multicast delegate invoked when the client successfully connects.
     *
     * Use `.add()` to subscribe callbacks, which returns a DelegateHandle
     * that can be used to unsubscribe later.
     *
     * @param isConnected True if the client is connected.
     */
    using OnConnect = MulticastDelegate<void(bool isConnected)>;

    /**
     * @brief Multicast delegate invoked when the client disconnects.
     *
     * Use `.add()` to subscribe callbacks, which returns a DelegateHandle
     * that can be used to unsubscribe later.
     *
     * @param isDisconnected True if the client is disconnected gracefully.
     */
    using OnDisconnect = MulticastDelegate<void(bool isDisconnected)>;

    /**
     * @brief Multicast delegate invoked after a publishing attempt completes.
     *
     * Use `.add()` to subscribe callbacks, which returns a DelegateHandle
     * that can be used to unsubscribe later.
     *
     * @param isPublished True if the message was published successfully (per QoS rules).
     */
    using OnPublish = MulticastDelegate<void(bool isPublished)>;

    /**
     * @brief Multicast delegate invoked when a message is received.
     *
     * Use `.add()` to subscribe callbacks, which returns a DelegateHandle
     * that can be used to unsubscribe later.
     *
     * @param message The received message.
     */
    using OnMessage = MulticastDelegate<void(const Message& message)>;

    /**
     * @brief Multicast delegate invoked after a subscribe operation completes.
     *
     * Use `.add()` to subscribe callbacks, which returns a DelegateHandle
     * that can be used to unsubscribe later.
     *
     * @param resultsPerTopic Results per topic filter in the subscribe request.
     */
    using OnSubscribe = MulticastDelegate<void(const std::shared_ptr<std::vector<SubscribeResult>>& resultsPerTopic)>;

    /**
     * @brief Multicast delegate invoked after an unsubscribe operation completes.
     *
     * Use `.add()` to subscribe callbacks, which returns a DelegateHandle
     * that can be used to unsubscribe later.
     *
     * @param resultsPerTopic Results per topic filter in the unsubscribe request.
     */
    using OnUnsubscribe = MulticastDelegate<void(const std::shared_ptr<std::vector<UnsubscribeResult>>& resultsPerTopic)>;
} // namespace reactormq::mqtt
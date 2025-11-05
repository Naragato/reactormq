#pragma once

#include <atomic>
#include <cstddef>
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

    enum class DisconnectPolicy { OnDestruct, Manual };

    /**
     * @brief Handle to a delegate subscription that can be disconnected.
     *
     * Represents a handle to a MulticastDelegate. When the DelegateHandle
     * is destroyed or disconnect() is called (per policy), the associated
     * callback may be removed from the delegate.
     */
    class DelegateHandle
    {
    public:
        /**
         * @brief Default constructor creates an invalid hande.
         */
        DelegateHandle() = default;

        /**
         * @brief Constructor.
         * @param owner Weak reference to the delegate's lifetime.
         * @param token Weak reference to the slot's lifetime.
         * @param disconnect Function to disconnect this handle.
         * @param policy Whether the handle disconnects on destruction or only manually.
         */
        DelegateHandle(
            std::weak_ptr<void> owner,
            std::weak_ptr<void> token,
            std::function<void()> disconnect,
            const DisconnectPolicy policy)
            : m_owner(std::move(owner)),
              m_token(std::move(token)),
              m_disconnect(std::move(disconnect)),
              m_policy(policy)
        {
        }

        ~DelegateHandle()
        {
            if (m_policy == DisconnectPolicy::OnDestruct)
                disconnect();
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
         * @brief Check if this handle is still valid.
         * @return True if the delegate is still alive and this handle is active.
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

    /**
     * @brief A thread-safe multicast delegate supporting multiple subscribers.
     *
     * MulticastDelegate allows multiple callbacks to be registered and invoked
     * in order when broadcast() is called. Supports both void and non-void
     * return types, member functions, and automatic cleanup of expired weak
     * references.
     *
     * @tparam Signature Function signature in the form R(Args...).
     */
    template<class Signature>
    class MulticastDelegate;

    template<class R, class... Args>
    class MulticastDelegate<R(Args...)>
    {
        using OptionalR = std::conditional_t<std::is_void_v<R>, std::monostate, R>;

        struct SlotBase
        {
            std::size_t id{};
            std::function<std::optional<OptionalR>(Args...)> call;
            std::function<bool()> expired;
            std::shared_ptr<void> keeper;
            std::shared_ptr<void> token;
        };

    public:
        /**
         * @brief Constructor.
         */
        MulticastDelegate()
            : m_nextId(1), m_self(std::make_shared<int>(0))
        {
        }

        /**
         * @brief Add a free function pointer or lambda by wrapping it into a shared_ptr.
         * @tparam F Function type.
         * @param f Function or lambda to add.
         * @return DelegateHandle that can be used to disconnect the callback.
         */
        template<class F>
        DelegateHandle add(F&& f)
        {
            using Fn = std::decay_t<F>;
            static_assert(
                std::is_invocable_r_v<R, Fn&, Args...>,
                "Callable must be invocable with Args... and return R");

            auto sp = std::make_shared<Fn>(std::forward<F>(f));
            auto wsp = std::weak_ptr<Fn>(sp);

            SlotBase slot;
            slot.id = m_nextId++;
            slot.token = std::make_shared<int>(0);
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
                    if (auto sp = wsp.lock())
                    {
                        (*sp)(std::forward<Args>(args)...);
                        return std::optional<OptionalR>{ std::monostate{} };
                    }
                    return std::nullopt;
                };
            }
            else
            {
                slot.call = [wsp](Args... args) -> std::optional<OptionalR>
                {
                    if (auto sp = wsp.lock())
                    {
                        return std::optional<OptionalR>((*sp)(std::forward<Args>(args)...));
                    }
                    return std::nullopt;
                };
            }

            auto id = addSlot(std::move(slot));
            return DelegateHandle(
                m_self, wtoken, [this, id]
                {
                    this->remove(id);
                }, DisconnectPolicy::Manual);
        }

        /**
         * @brief Add a callable stored in shared_ptr<Callable> where Callable: R(Args...).
         * @tparam Callable Type of the callable object.
         * @param cb Shared pointer to the callable object.
         * @return DelegateHandle that can be used to disconnect the callback.
         */
        template<class Callable>
        DelegateHandle addCallable(const std::shared_ptr<Callable>& cb)
        {
            static_assert(
                std::is_invocable_r_v<R, Callable&, Args...>,
                "Callable must be invocable with Args... and return R");
            auto wcb = std::weak_ptr<Callable>(cb);

            SlotBase slot;
            slot.id = m_nextId++;
            slot.token = std::make_shared<int>(0);
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
                        (*sp)(std::forward<Args>(args)...);
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
                        return std::optional<OptionalR>((*sp)(std::forward<Args>(args)...));
                    }
                    return std::nullopt;
                };
            }

            auto handle = addSlot(std::move(slot));
            return DelegateHandle(
                m_self, wtoken, [this, id = handle]
                {
                    this->remove(id);
                }, DisconnectPolicy::Manual);
        }

        /**
         * @brief Add a member function callback.
         * @tparam T Object type.
         * @tparam MemFn Member function pointer type.
         * @param obj Shared pointer to the owner, which controls the lifetime of the handle
         * @param mf Member function pointer.
         * @return DelegateHandle that can be used to disconnect the callback.
         */
        template<class T, class MemFn>
        DelegateHandle addMember(const std::shared_ptr<T>& obj, MemFn mf)
        {
            static_assert(
                std::is_invocable_r_v<R, MemFn, T&, Args...> ||
                std::is_invocable_r_v<R, MemFn, T*, Args...>,
                "Member function must be callable with (T&, Args...) or (T*, Args...)");

            auto wobj = std::weak_ptr<T>(obj);

            SlotBase slot;
            slot.id = m_nextId++;
            slot.token = std::make_shared<int>(0);
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
                            (sp.get()->*mf)(std::forward<Args>(args)...);
                        }
                        else
                        {
                            mf(*sp, std::forward<Args>(args)...);
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
                            return std::optional<OptionalR>((sp.get()->*mf)(std::forward<Args>(args)...));
                        }
                        else
                        {
                            return std::optional<OptionalR>(mf(*sp, std::forward<Args>(args)...));
                        }
                    }
                    return std::nullopt;
                };
            }

            auto handle = addSlot(std::move(slot));
            return DelegateHandle(
                m_self, wtoken, [this, id = handle]
                {
                    this->remove(id);
                }, DisconnectPolicy::Manual);
        }

        /**
         * @brief Add a callable via shared_ptr (alias of addCallable).
         * @tparam Callable Type of the callable object.
         * @param cb Shared pointer to the callable object.
         * @return DelegateHandle that can be used to disconnect the callback.
         */
        template<class Callable>
        DelegateHandle add(const std::shared_ptr<Callable>& cb)
        {
            return addCallable(cb);
        }

        /**
         * @brief Remove a callback by its handle id.
         * @param id The id of the callback to remove.
         */
        void remove(std::size_t id) noexcept
        {
            std::scoped_lock lock(m_mutex);
            for (auto it = m_slots.begin(); it != m_slots.end(); ++it)
            {
                if (it->id == id)
                {
                    it->token.reset();
                    m_slots.erase(it);
                    break;
                }
            }
        }

        /**
         * @brief Clear all callbacks.
         */
        void clear() noexcept
        {
            std::scoped_lock lock(m_mutex);
            for (auto& s : m_slots)
                s.token.reset();
            m_slots.clear();
        }

        /**
         * @brief Broadcast to all registered callbacks.
         *
         * For void return type: invokes all callbacks and returns void.
         * For non-void return type: invokes all callbacks and returns a vector
         * of results from live callbacks in registration order.
         *
         * @param args Arguments to forward to each callback.
         * @return void for R=void, or std::vector<R> containing results from live callbacks.
         */
        std::conditional_t<std::is_void_v<R>, void, std::vector<R>>
        broadcast(Args... args)
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
                    auto res = s.call(std::forward<Args>(args)...);
                    (void)res;
                }
                return;
            }
            else
            {
                std::vector<R> out;
                out.reserve(snapshot.size());
                for (auto& s : snapshot)
                {
                    if (auto res = s.call(std::forward<Args>(args)...))
                    {
                        out.push_back(std::move(*res));
                    }
                }
                return out;
            }
        }

        /**
         * @brief Get the count of live slots (approximate; pruned on next broadcast or add/remove).
         * @return Number of registered callbacks.
         */
        std::size_t getSize() const noexcept
        {
            std::scoped_lock lock(m_mutex);
            return m_slots.size();
        }

    private:
        std::size_t addSlot(SlotBase&& slot)
        {
            std::scoped_lock lock(m_mutex);
            m_slots.push_back(std::move(slot));
            return m_slots.back().id;
        }

        mutable std::mutex m_mutex;
        std::vector<SlotBase> m_slots;
        std::atomic<std::size_t> m_nextId;
        std::shared_ptr<void> m_self;
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
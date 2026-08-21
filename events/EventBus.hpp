#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <mutex>

namespace ember::events {

// Base interface for type erasure
struct AnyHandler {
    virtual ~AnyHandler() = default;
};

// Wrapper holding typed callback lists. Each handler is stored behind a
// shared_ptr so publish() can copy the (cheap, refcounted) pointer list
// under lock instead of deep-copying every std::function on every publish.
template <typename T>
struct HandlerList : public AnyHandler {
    struct Entry {
        std::size_t id;
        std::shared_ptr<std::function<void(const T&)>> fn;
    };
    std::vector<Entry> handlers;
};

class EventBus {
public:
    using SubscriptionId = std::size_t;

    EventBus() = default;
    ~EventBus() = default;

    // Prevent copying
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // Subscribe a callback to a specific EventType. Returns an id that can
    // be passed to unsubscribe<EventType>() — hold onto it if the callback
    // captures anything whose lifetime doesn't outlive the EventBus.
    template <typename EventType>
    SubscriptionId subscribe(std::function<void(const EventType&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type_idx = std::type_index(typeid(EventType));

        auto& entry = subscribers_[type_idx];
        if (!entry) {
            entry = std::make_unique<HandlerList<EventType>>();
        }

        auto* list = static_cast<HandlerList<EventType>*>(entry.get());
        SubscriptionId id = next_id_++;
        list->handlers.push_back({id, std::make_shared<std::function<void(const EventType&)>>(std::move(callback))});
        return id;
    }

    // Remove a single subscription previously returned by subscribe<EventType>().
    template <typename EventType>
    void unsubscribe(SubscriptionId id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type_idx = std::type_index(typeid(EventType));

        auto it = subscribers_.find(type_idx);
        if (it == subscribers_.end()) {
            return;
        }

        auto* list = static_cast<HandlerList<EventType>*>(it->second.get());
        auto& handlers = list->handlers;
        handlers.erase(
            std::remove_if(handlers.begin(), handlers.end(),
                            [id](const auto& e) { return e.id == id; }),
            handlers.end());
    }

    // Publish an event instance to all matching subscribers
    template <typename EventType>
    void publish(const EventType& event) {
        std::vector<std::shared_ptr<std::function<void(const EventType&)>>> callbacks_copy;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto type_idx = std::type_index(typeid(EventType));

            auto it = subscribers_.find(type_idx);
            if (it != subscribers_.end()) {
                auto* list = static_cast<HandlerList<EventType>*>(it->second.get());
                callbacks_copy.reserve(list->handlers.size());
                for (const auto& entry : list->handlers) {
                    callbacks_copy.push_back(entry.fn);
                }
            }
        }

        // Execute handlers outside of lock section to prevent deadlocks
        for (const auto& handler : callbacks_copy) {
            if (handler && *handler) {
                (*handler)(event);
            }
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::type_index, std::unique_ptr<AnyHandler>> subscribers_;
    std::size_t next_id_{0};
};

} // namespace ember::events
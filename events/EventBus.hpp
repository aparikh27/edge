#pragma once

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

// Wrapper holding typed callback lists
template <typename T>
struct HandlerList : public AnyHandler {
    std::vector<std::function<void(const T&)>> handlers;
};

class EventBus {
public:
    EventBus() = default;
    ~EventBus() = default;

    // Prevent copying
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // Subscribe a callback to a specific EventType
    template <typename EventType>
    void subscribe(std::function<void(const EventType&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type_idx = std::type_index(typeid(EventType));

        if (subscribers_.find(type_idx) == subscribers_.end()) {
            subscribers_[type_idx] = std::make_unique<HandlerList<EventType>>();
        }

        auto* list = static_cast<HandlerList<EventType>*>(subscribers_[type_idx].get());
        list->handlers.push_back(callback);
    }

    // Publish an event instance to all matching subscribers
    template <typename EventType>
    void publish(const EventType& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type_idx = std::type_index(typeid(EventType));

        auto it = subscribers_.find(type_idx);
        if (it != subscribers_.end()) {
            auto* list = static_cast<HandlerList<EventType>*>(it->second.get());
            for (const auto& handler : list->handlers) {
                handler(event);
            }
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::type_index, std::unique_ptr<AnyHandler>> subscribers_;
};

} // namespace ember::events
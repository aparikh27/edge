#pragma once
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <memory>

namespace ember::events {
    // Base interface (Type Erasure Anchor)
struct AnyHandler {
    virtual ~AnyHandler() = default;
};

// Derived template struct holding actual typed callbacks
template <typename T>
struct HandlerList : public AnyHandler {
    std::vector<std::function<void(const T&)>> handlers;
};
    class EventBus {
        public:
            template <typename EventType>
            void publish(const EventType& event);
            template <typename EventType>
            void subscribe(std::function<void(const EventType&)> callback);
        private:
            std::unordered_map<std::type_index, std::unique_ptr<AnyHandler>> subscribers_;




    };

}
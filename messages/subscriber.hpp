#pragma once

#include <memory>
#include <string>
#include "message.hpp"
#include "threads.hpp"

namespace ember::messaging {

class Subscriber {
public:
    using QueueType = ThreadSafeQueue<std::shared_ptr<const Message>>;

    explicit Subscriber(std::string topic)
        : m_topic(std::move(topic)), m_queue(std::make_shared<QueueType>()) {}

    [[nodiscard]] const std::string& get_topic() const { return m_topic; }
    [[nodiscard]] std::shared_ptr<QueueType> get_queue() const { return m_queue; }

    // How thread worker loops read messages safely. The queue carries a
    // shared, immutable Message (Coordinator fans one out to every matching
    // subscriber instead of deep-copying it per subscriber); pop() makes the
    // one copy the caller actually needs.
    bool pop(Message& msg) {
        std::shared_ptr<const Message> ptr;
        if (!m_queue->try_pop(ptr)) return false;
        msg = *ptr;
        return true;
    }

    bool wait_and_pop(Message& msg) {
        std::shared_ptr<const Message> ptr;
        if (!m_queue->wait_and_pop(ptr)) return false;
        msg = *ptr;
        return true;
    }

    void stop() { if (m_queue) m_queue->shutdown(); }

    void alert();

private:
    std::string m_topic;
    std::shared_ptr<QueueType> m_queue;
};

} // namespace ember::messaging
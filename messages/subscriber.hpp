#pragma once

#include <memory>
#include <string>
#include "message.hpp"
#include "threads.hpp"

namespace ember::messaging {

class Subscriber {
public:
    explicit Subscriber(std::string topic) 
        : m_topic(std::move(topic)), m_queue(std::make_shared<ThreadSafeQueue<Message>>()) {}

    [[nodiscard]] const std::string& get_topic() const { return m_topic; }
    [[nodiscard]] std::shared_ptr<ThreadSafeQueue<Message>> get_queue() const { return m_queue; }

    // How thread worker loops read messages safely
    bool pop(Message& msg) { return m_queue->try_pop(msg); }
    bool wait_and_pop(Message& msg) { return m_queue->wait_and_pop(msg); }
    void stop() { if (m_queue) m_queue->shutdown(); }

    void alert();

private:
    std::string m_topic;
    std::shared_ptr<ThreadSafeQueue<Message>> m_queue;
};

} // namespace ember::messaging
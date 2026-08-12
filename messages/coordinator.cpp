#include "coordinator.hpp"

namespace ember::messaging {

void Coordinator::subscribe(const std::shared_ptr<Subscriber>& s) {
    if (!s) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscribers.push_back(s);
}

void Coordinator::publish(const Message& m) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& subscriber : m_subscribers) {
        if (subscriber && subscriber->get_topic() == m.topic) {
            // Non-blocking: pushes data into the subscriber's private ThreadSafeQueue!
            subscriber->get_queue()->push(m);
        }
    }
}

} // namespace ember::messaging
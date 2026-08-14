#include "coordinator.hpp"

namespace ember::messaging {

void Coordinator::subscribe(const std::shared_ptr<Subscriber>& s) {
    if (!s) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscribers.push_back(s);
}

void Coordinator::publish(const Message& m) {
    std::vector<std::shared_ptr<Subscriber>> targets;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& subscriber : m_subscribers) {
            if (subscriber && subscriber->get_topic() == m.topic) {
                targets.push_back(subscriber);
            }
        }
    }

    for (const auto& subscriber : targets) {
        if (auto q = subscriber->get_queue()) {
            q->push(m);
        }
    }
}

} // namespace ember::messaging
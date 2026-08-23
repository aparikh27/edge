#include "coordinator.hpp"

#include <algorithm>

namespace ember::messaging {

void Coordinator::subscribe(const std::shared_ptr<Subscriber>& s) {
    if (!s) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscribers.push_back(s);
}

void Coordinator::unsubscribe(const std::shared_ptr<Subscriber>& s) {
    if (!s) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscribers.erase(std::remove(m_subscribers.begin(), m_subscribers.end(), s), m_subscribers.end());
}

void Coordinator::publish(const Message& m) {
    // Fan out a single shared, immutable copy instead of deep-copying the
    // Message (topic + payload strings) once per matching subscriber.
    auto shared_msg = std::make_shared<const Message>(m);

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
            q->push(shared_msg);
        }
    }
}

} // namespace ember::messaging
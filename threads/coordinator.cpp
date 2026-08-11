#include "coordinator.hpp"

namespace ember::messaging {

void Coordinator::publish(const Message& m) {
    notify(m);
}

void Coordinator::subscribe(const Subscriber& s) {
    m_subscribers.push_back(s);
}

void Coordinator::notify(const Message& m) {
    for (auto& subscriber : m_subscribers) {
        if (subscriber.topic == m.topic) {
            subscriber.alert();
        }
    }
}

} // namespace ember::messaging

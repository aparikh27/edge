#include "publisher.hpp"

namespace ember::messaging {

Publisher::Publisher(Coordinator* c) 
    : m_coordinator(c) {}

void Publisher::push(const Message& m) {
    if (m_coordinator != nullptr) {
        m_coordinator->publish(m);
    }
}

} // namespace ember::messaging
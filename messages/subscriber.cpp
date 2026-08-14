#include "subscriber.hpp"
#include <iostream>

namespace ember::messaging {

void Subscriber::alert() {
    std::cout << "Subscriber notified for topic: " << m_topic << '\n';
}

} // namespace ember::messaging

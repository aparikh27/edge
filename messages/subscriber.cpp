#include "subscriber.hpp"
#include <iostream>

namespace ember::messaging {

void Subscriber::alert() {
    std::cout << "Subscriber notified for topic: " << topic << '\n';
}

} // namespace ember::messaging

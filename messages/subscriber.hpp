#pragma once

#include <string>

namespace ember::messaging {

class Subscriber {
public:
    std::string topic;
    void alert();
};

} // namespace ember::messaging

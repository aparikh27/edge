#pragma once

#include <vector>
#include "message.hpp"
#include "subscriber.hpp"

namespace ember::messaging {

class Coordinator {
public:
    void publish(const Message& m);
    void subscribe(const Subscriber& s);
    void notify(const Message& m);

private:
    std::vector<Subscriber> m_subscribers;
};

} // namespace ember::messaging

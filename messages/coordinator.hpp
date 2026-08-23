#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include "message.hpp"
#include "subscriber.hpp"

namespace ember::messaging {

class Coordinator {
public:
    Coordinator() = default;

    // Prevent copies
    Coordinator(const Coordinator&) = delete;
    Coordinator& operator=(const Coordinator&) = delete;

    void publish(const Message& m);
    void subscribe(const std::shared_ptr<Subscriber>& s);
    void unsubscribe(const std::shared_ptr<Subscriber>& s);

private:
    mutable std::mutex m_mutex;
    std::vector<std::shared_ptr<Subscriber>> m_subscribers;
};

} // namespace ember::messaging
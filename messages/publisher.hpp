#pragma once

#include "coordinator.hpp"
#include "message.hpp"

namespace ember::messaging {

class Publisher {
public:
    explicit Publisher(Coordinator* c);
    
    void push(const Message& m);

private:
    Coordinator* m_coordinator;
};

} // namespace ember::messaging
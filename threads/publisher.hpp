#pragma once
#include "coordinator.hpp"
#include "message.hpp"

class Publisher {
    public:
        Publisher(Coordinator* c);
        void push(Message m);
    private:
        Coordinator* m_coordinator;

};
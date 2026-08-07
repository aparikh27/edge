#pragma once

#include "task.hpp"
#include <chrono>
#include <functional>
#include <string>
#include <vector>

class Scheduler {
public:
    void schedule(const std::string& name, std::chrono::microseconds period, std::function<void()> callback);
    void tick();
    void clear();
    size_t task_count() const;

private:
    std::vector<Task> m_tasks;
};
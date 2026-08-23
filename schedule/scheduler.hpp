#pragma once

#include "task.hpp"
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace ember::schedule {

class Scheduler {
public:
    void schedule(const std::string& name, std::chrono::microseconds period, std::function<void()> callback);
    void tick();
    void clear();
    [[nodiscard]] size_t task_count() const;

private:
    mutable std::mutex m_mutex;
    std::vector<Task> m_tasks;
};

} // namespace ember::schedule
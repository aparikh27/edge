#include "scheduler.hpp"
#include <chrono>
#include <utility>

void Scheduler::schedule(const std::string& name, std::chrono::microseconds period, std::function<void()> callback) {
    auto now = std::chrono::steady_clock::now();
    Task t(name, 0, 0, period, now, now + period, std::move(callback));
    m_tasks.push_back(std::move(t));
}

void Scheduler::tick() {
    auto now = std::chrono::steady_clock::now();

    for (auto& task : m_tasks) {
        task.execute(now);
    }
}

void Scheduler::clear() {
    m_tasks.clear();
}

size_t Scheduler::task_count() const {
    return m_tasks.size();
}
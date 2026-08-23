#include "scheduler.hpp"
#include <chrono>
#include <utility>

namespace ember::schedule {

void Scheduler::schedule(const std::string& name, std::chrono::microseconds period, std::function<void()> callback) {
    auto now = std::chrono::steady_clock::now();
    Task t(name, 0, 0, period, now, now + period, std::move(callback));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.push_back(std::move(t));
}

void Scheduler::tick() {
    auto now = std::chrono::steady_clock::now();

    // Held for the full iteration: task state (last/next run time) lives
    // inline in m_tasks, so it can't be snapshotted and executed unlocked
    // the way EventBus/Coordinator copy their handler lists. A task
    // callback that itself calls schedule()/clear() on this Scheduler
    // would deadlock (non-recursive mutex) — none of the current tasks do.
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& task : m_tasks) {
        task.execute(now);
    }
}

void Scheduler::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.clear();
}

size_t Scheduler::task_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks.size();
}

} // namespace ember::schedule
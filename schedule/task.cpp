#include "task.hpp"
#include <utility>

Task::Task(const std::string& name,
           int missed_deadlines,
           int execution_count,
           std::chrono::microseconds period,
           std::chrono::steady_clock::time_point last_run_time,
           std::chrono::steady_clock::time_point next_run_time,
           std::function<void()> callback)
    : m_name(name)
    , m_missed_deadlines(missed_deadlines)
    , m_execution_count(execution_count)
    , m_period(period)
    , m_last_run_time(last_run_time)
    , m_next_run_time(next_run_time)
    , m_callback(std::move(callback))
{
}

bool Task::is_due(std::chrono::steady_clock::time_point now) const {
    return now >= m_next_run_time;
}

bool Task::execute(std::chrono::steady_clock::time_point now) {
    if (!is_due(now)) {
        return false;
    }

    // Check if execution was significantly delayed past due time
    if (now > m_next_run_time + std::chrono::microseconds(100)) {
        m_missed_deadlines++;
    }

    if (m_callback) {
        m_callback();
    }

    m_last_run_time = now;
    // Maintain phase alignment across periodic executions
    m_next_run_time += m_period;
    if (m_next_run_time < now) {
        m_next_run_time = now + m_period;
    }
    m_execution_count++;
    return true;
}
#pragma once

#include <chrono>
#include <functional>
#include <string>

class Task {
public:
    Task(const std::string& name,
         int missed_deadlines,
         int execution_count,
         std::chrono::microseconds period,
         std::chrono::steady_clock::time_point last_run_time,
         std::chrono::steady_clock::time_point next_run_time,
         std::function<void()> callback);

    bool is_due(std::chrono::steady_clock::time_point now) const;
    bool execute(std::chrono::steady_clock::time_point now);

private:
    std::string m_name;
    int m_missed_deadlines;
    int m_execution_count;
    std::chrono::microseconds m_period;
    std::chrono::steady_clock::time_point m_last_run_time;
    std::chrono::steady_clock::time_point m_next_run_time;
    std::function<void()> m_callback;
};

#include "task.hpp"

Task::Task(string name, int missed_deadlines, int execution_count, chrono::microseconds period,
        chrono::steady_clock::time_point last_run_time, chrono::steady_clock::time_point next_run_time, function<void()> callback) {
            string m_name = name;
            int m_missed_deadlines = missed_deadlines;
            int m_execution_count = execution_count;
            chrono::microseconds m_period = period;
            chrono::steady_clock::time_point m_last_run_time = last_run_time;
            chrono::steady_clock::time_point m_next_run_time = next_run_time;
            function<void()> m_callback = callback;
        }


bool Task::is_due(chrono::steady_clock::time_point now) const {
    if (now >= next_run_time) {
        return true;
    } else {
        return false;
    }

}

bool Task::execute(chrono::steady_clock::time_point now) {
    if (is_due(now)) {
        callback();
        last_run_time = now;
        next_run_time = now + period;
        execution_count++;
        return true;
    } else {
        return false;
    }
}
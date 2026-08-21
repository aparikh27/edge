#pragma once

#include "../schedule/scheduler.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

namespace ember::runtime {

enum class State {
    Uninitialized,
    Initialized,
    Running,
    Stopping,
    Stopped,
    Error
};

struct RuntimeConfig {
    uint32_t target_frequency_hz{100};                     // Execution rate in Hz
    uint8_t verbosity_level{1};                            // Log verbosity
    std::chrono::milliseconds shutdown_timeout{1000};      // Graceful timeout duration
};

class Runtime {
public:
    // Initialize directly via constructor (RAII)
    explicit Runtime(const RuntimeConfig& config);
    ~Runtime();

    // Prevent copying to ensure a single instance per process lifecycle
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    // Public Lifecycle Interface
    void run();
    void step();

    // Requests the run() loop (expected to be executing on another thread)
    // to stop, and blocks until it has actually reached State::Stopped or
    // config_.shutdown_timeout elapses — closes the window where a caller
    // could destroy/reuse the Runtime while run() is still mid-iteration.
    void stop();

    // Register periodic work with the Runtime's internal scheduler. Safe to
    // call before run() starts; avoid calling it from inside a scheduled
    // task's own callback (Scheduler's lock is non-recursive).
    void schedule_task(const std::string& name, std::chrono::microseconds period, std::function<void()> callback);

    // Accessors
    [[nodiscard]] State get_state() const noexcept;
    [[nodiscard]] bool is_running() const noexcept;

private:
    std::atomic<State> current_state_{State::Uninitialized};
    RuntimeConfig config_{};
    schedule::Scheduler scheduler_{};

    std::mutex state_mutex_;
    std::condition_variable state_cv_;
};

} // namespace ember::runtime
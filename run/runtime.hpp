#pragma once

#include "../schedule/scheduler.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>

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
    void stop();

    // Accessors
    [[nodiscard]] State get_state() const noexcept;
    [[nodiscard]] bool is_running() const noexcept;

private:
    std::atomic<State> current_state_{State::Uninitialized};
    RuntimeConfig config_{};
    Scheduler scheduler_{};
};

} // namespace ember::runtime
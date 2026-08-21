#include "runtime.hpp"
#include "log.hpp"
#include "time.hpp"
#include "../schedule/scheduler.hpp"

#include <stdexcept>

namespace ember::runtime {

Runtime::Runtime(const RuntimeConfig& config)
    : config_(config) {
    log::log_message(log::Level::Info, "Initializing Runtime kernel...");

    if (config_.target_frequency_hz == 0) {
        current_state_ = State::Error;
        log::log_message(log::Level::Fatal, "Target frequency cannot be 0 Hz.");
        throw std::invalid_argument("Target frequency must be greater than 0 Hz.");
    }

    // Initialization successful
    current_state_ = State::Initialized;
    log::log_message(log::Level::Info, "Runtime kernel successfully initialized.");
}

Runtime::~Runtime() {
    if (current_state_ == State::Running || current_state_ == State::Stopping) {
        stop();
    }
}

void Runtime::run() {
    if (current_state_ != State::Initialized && current_state_ != State::Stopped) {
        log::log_message(log::Level::Warning, "Cannot start runtime: Kernel is not initialized or stopped.");
        return;
    }

    current_state_ = State::Running;
    log::log_message(log::Level::Info, "Runtime kernel loop started.");

    time::Rate rate_limiter(config_.target_frequency_hz);

    while (current_state_ == State::Running) {
        step();
        rate_limiter.sleep();
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        current_state_ = State::Stopped;
    }
    state_cv_.notify_all();
    log::log_message(log::Level::Info, "Runtime kernel loop finished.");
}

void Runtime::step() {
    if (current_state_ != State::Running) {
        return;
    }

    scheduler_.tick();
}

void Runtime::stop() {
    // Any caller observing Running or Stopping waits for Stopped below —
    // not just whichever caller happens to win the CAS — so a second,
    // concurrent stop() (including one made from the destructor) still
    // blocks for real confirmation instead of returning immediately.
    const State observed = current_state_.load();
    if (observed != State::Running && observed != State::Stopping) {
        return; // Nothing to stop.
    }

    State expected = State::Running;
    if (current_state_.compare_exchange_strong(expected, State::Stopping)) {
        log::log_message(log::Level::Info, "Stopping Runtime kernel...");
    }

    std::unique_lock<std::mutex> lock(state_mutex_);
    const bool stopped = state_cv_.wait_for(lock, config_.shutdown_timeout, [this] {
        return current_state_.load() == State::Stopped;
    });

    if (!stopped) {
        log::log_message(log::Level::Warning,
            "Runtime did not reach State::Stopped within shutdown_timeout; "
            "the run() loop may still be executing on its thread.");
    }
}

void Runtime::schedule_task(const std::string& name, std::chrono::microseconds period, std::function<void()> callback) {
    scheduler_.schedule(name, period, std::move(callback));
}

State Runtime::get_state() const noexcept {
    return current_state_;
}

bool Runtime::is_running() const noexcept {
    return current_state_ == State::Running;
}

} // namespace ember::runtime
#include "runtime.hpp"
#include "log.hpp"
#include "runtime/time.hpp"

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
    if (current_state_ == State::Running) {
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

    current_state_ = State::Stopped;
    log::log_message(log::Level::Info, "Runtime kernel loop finished.");
}

void Runtime::step() {
    if (current_state_ != State::Running) {
        return;
    }

    // Kernel tick logic goes here (e.g., dispatching multi-agent tasks)
}

void Runtime::stop() {
    if (current_state_ == State::Running) {
        log::log_message(log::Level::Info, "Stopping Runtime kernel...");
        current_state_ = State::Stopping;
    }
}

State Runtime::get_state() const noexcept {
    return current_state_;
}

bool Runtime::is_running() const noexcept {
    return current_state_ == State::Running;
}

} // namespace ember::runtime
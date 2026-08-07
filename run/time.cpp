#include "time.hpp"

namespace ember::time {

// --- Free Utility Functions ---

std::chrono::steady_clock::time_point now() noexcept {
	return std::chrono::steady_clock::now();
}

uint64_t now_ms() noexcept {
	const auto duration = std::chrono::steady_clock::now().time_since_epoch();
	return static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

uint64_t now_us() noexcept {
	const auto duration = std::chrono::steady_clock::now().time_since_epoch();
	return static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
}

void sleep_for_ms(uint32_t milliseconds) {
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void sleep_for_us(uint32_t microseconds) {
	std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

// --- Stopwatch Implementation ---

Stopwatch::Stopwatch() {
	reset();
}

void Stopwatch::start() noexcept {
	start_time_ = now();
	running_ = true;
}

void Stopwatch::stop() noexcept {
	if (running_) {
		stop_time_ = now();
		running_ = false;
	}
}

void Stopwatch::reset() noexcept {
	start_time_ = std::chrono::steady_clock::time_point{};
	stop_time_ = std::chrono::steady_clock::time_point{};
	running_ = false;
}

uint64_t Stopwatch::elapsed_ms() const noexcept {
	const auto end = running_ ? now() : stop_time_;
	return static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time_).count());
}

uint64_t Stopwatch::elapsed_us() const noexcept {
	const auto end = running_ ? now() : stop_time_;
	return static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::microseconds>(end - start_time_).count());
}

bool Stopwatch::is_running() const noexcept {
	return running_;
}

// --- Rate / Dynamic Sleeping Implementation ---

Rate::Rate(uint32_t target_hz)
	: target_period_(target_hz > 0 ? 1'000'000 / target_hz : 1'000'000),
	  last_time_(now()) {}

void Rate::sleep() {
	const auto current_time = now();
	const auto elapsed =
		std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_time_);

	if (elapsed < target_period_) {
		std::this_thread::sleep_for(target_period_ - elapsed);
	}

	last_time_ = now();
}

void Rate::reset() noexcept {
	last_time_ = now();
}

} // namespace ember::time
#pragma once

#include <chrono>
#include <cstdint>
#include <thread>

namespace ember::time {

// Free Utility Functions
[[nodiscard]] std::chrono::steady_clock::time_point now() noexcept;
[[nodiscard]] uint64_t now_ms() noexcept;
[[nodiscard]] uint64_t now_us() noexcept;

void sleep_for_ms(uint32_t milliseconds);
void sleep_for_us(uint32_t microseconds);

// Stopwatch Class
class Stopwatch {
public:
	Stopwatch();

	void start() noexcept;
	void stop() noexcept;
	void reset() noexcept;

	[[nodiscard]] uint64_t elapsed_ms() const noexcept;
	[[nodiscard]] uint64_t elapsed_us() const noexcept;
	[[nodiscard]] bool is_running() const noexcept;

private:
	std::chrono::steady_clock::time_point start_time_{};
	std::chrono::steady_clock::time_point stop_time_{};
	bool running_{false};
};

// Rate / Dynamic Sleeping Helper Class
class Rate {
public:
	explicit Rate(uint32_t target_hz);

	void sleep();
	void reset() noexcept;

private:
	std::chrono::microseconds target_period_{};
	std::chrono::steady_clock::time_point last_time_{};
};

} // namespace ember::time
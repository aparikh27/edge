#pragma once

#include <string>
#include <string_view>

namespace ember::log {

// Scoped Enum for Log Severity Levels
enum class Level {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

// Converts Level enum to printable string representation
[[nodiscard]] std::string_view level_to_string(Level level) noexcept;

// Core log string generator
[[nodiscard]] std::string generate_log(Level level, std::string_view message);

// Console output utility function
void log_message(Level level, std::string_view message);

} // namespace ember::log
#include "log.hpp"
#include "time.hpp"

#include <iostream>
#include <format> // Standard C++20 formatting library

namespace ember::log {

std::string_view level_to_string(Level level) noexcept {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error:   return "ERROR";
        case Level::Fatal:   return "FATAL";
        default:             return "UNKNOWN";
    }
}

std::string generate_log(Level level, std::string_view message) {
    // Grab the current timestamp in milliseconds 
    const uint64_t timestamp = ember::time::now_ms();
    
    // Format into standard log format: [TIMESTAMP_MS] [LEVEL] Message
    return std::format("[{}] [{}] {}", timestamp, level_to_string(level), message);
}

void log_message(Level level, std::string_view message) {
    const std::string formatted_log = generate_log(level, message);
    
    // Direct fatal and error messages to stderr, otherwise stdout
    if (level == Level::Error || level == Level::Fatal) {
        std::cerr << formatted_log << '\n';
    } else {
        std::cout << formatted_log << '\n';
    }
}

} // namespace ember::log
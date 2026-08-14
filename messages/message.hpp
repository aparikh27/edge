#pragma once

#include <string>
#include <chrono>

namespace ember::messaging {

struct Message {
    std::string topic;
    std::chrono::steady_clock::time_point timestamp{std::chrono::steady_clock::now()};
    std::string payload;
};

} // namespace ember::messaging
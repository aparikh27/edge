#pragma once

#include <cstdint>
#include <span>

namespace ember::serialization {

inline uint16_t calculate_fletcher16(std::span<const uint8_t> data) {
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;

    for (uint8_t byte : data) {
        sum1 = (sum1 + byte) % 255;
        sum2 = (sum2 + sum1) % 255;
    }

    return (sum2 << 8) | sum1;
}

} // namespace ember::serialization
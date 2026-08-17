#pragma once

#include <cstdint> 
#include <cstring> 

namespace ember::serialization {

inline void write_u16_be(uint8_t* dest, uint16_t val) {
    dest[0] = static_cast<uint8_t>((val >> 8) & 0xFF);
    dest[1] = static_cast<uint8_t>(val & 0xFF);
}

inline uint16_t read_u16_be(const uint8_t* src) {
    return (static_cast<uint16_t>(src[0]) << 8) | static_cast<uint16_t>(src[1]);
}

inline void write_float_be(uint8_t* dest, float val) {
    uint32_t raw;
    std::memcpy(&raw, &val, sizeof(float));
    dest[0] = static_cast<uint8_t>((raw >> 24) & 0xFF);
    dest[1] = static_cast<uint8_t>((raw >> 16) & 0xFF);
    dest[2] = static_cast<uint8_t>((raw >> 8) & 0xFF);
    dest[3] = static_cast<uint8_t>(raw & 0xFF);
}

} // namespace ember::serialization
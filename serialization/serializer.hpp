#pragma once

#include "checksum.hpp"
#include "endian.hpp"

#include <cstdint> 
#include <cstring> 
#include <vector>
#include <span>

namespace ember::serialization {

struct PacketHeader {
    uint8_t magic[2]{'E', 'M'};
    uint8_t type{0};
    uint16_t payload_len{0};
};

class Serializer {
public:
    // Packs payload bytes into a fully framed, checksummed binary packet
    static std::vector<uint8_t> pack(uint8_t msg_type, std::span<const uint8_t> payload) {
        size_t total_size = 2 + 1 + 2 + payload.size() + 2; // Magic + Type + Len + Payload + Checksum
        std::vector<uint8_t> packet(total_size);

        // 1. Header
        packet[0] = 'E';
        packet[1] = 'M';
        packet[2] = msg_type;
        write_u16_be(&packet[3], static_cast<uint16_t>(payload.size()));

        // 2. Payload
        std::copy(payload.begin(), payload.end(), packet.begin() + 5);

        // 3. Checksum over Header + Payload
        auto check_region = std::span<const uint8_t>(packet.data(), 5 + payload.size());
        uint16_t checksum = calculate_fletcher16(check_region);
        write_u16_be(&packet[5 + payload.size()], checksum);

        return packet;
    }
};

} // namespace ember::serialization
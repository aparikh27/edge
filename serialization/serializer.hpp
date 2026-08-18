#pragma once

#include <cstdint> 
#include <cstring> 

namespace ember::serialization {
    enum class PacketHeader {
        uint8_t header[2]{'E', 'M'};
        uint8_t type{0};
        uint16_t payload_len{0};

    };

}
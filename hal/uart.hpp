#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace ember::hal {

class IUart {
public:
    virtual ~IUart() = default;

    virtual void open(uint32_t baud_rate) = 0;
    virtual void close() = 0;

    virtual void write(std::string_view data) = 0;
    virtual std::string read(size_t max_bytes) = 0;
    [[nodiscard]] virtual bool available() const = 0;
};

} // namespace ember::hal
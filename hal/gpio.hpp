#pragma once

#include <cstdint>

namespace ember::hal {

enum class PinMode {
    Input,
    Output
};

enum class PinState {
    Low = 0,
    High = 1
};

class IGpio {
public:
    virtual ~IGpio() = default;

    virtual void set_high() = 0;
    virtual void set_low() = 0;
    virtual void toggle() = 0;
    [[nodiscard]] virtual PinState read() const = 0;
};

} // namespace ember::hal
#pragma once

#include "device.hpp"
#include "../hal/gpio.hpp"
#include <memory>

namespace ember::device_manager {

class GpioDevice : public Device {
public:
    GpioDevice(std::string name, std::shared_ptr<hal::IGpio> gpio_hal)
        : Device(std::move(name)), m_gpio(std::move(gpio_hal)) {}

    bool initialize() override {
        if (!m_gpio) return false;
        set_state(DeviceState::Initialized);
        return true;
    }

    bool start() override {
        if (get_state() != DeviceState::Initialized && get_state() != DeviceState::Stopped) {
            return false;
        }
        set_state(DeviceState::Active);
        return true;
    }

    void stop() override {
        if (get_state() == DeviceState::Shutdown) return; // Shutdown is terminal
        if (m_gpio) {
            m_gpio->set_low(); // Safe default state
        }
        set_state(DeviceState::Stopped);
    }

    void shutdown() override {
        if (get_state() == DeviceState::Shutdown) return; // idempotent
        stop();
        set_state(DeviceState::Shutdown);
    }

    void reset() override {
        if (get_state() == DeviceState::Shutdown) return; // Shutdown is terminal; construct a new device instead
        stop();
        initialize();
    }

    // Access to underlying HAL for application logic
    [[nodiscard]] const std::shared_ptr<hal::IGpio>& get_hal() const { return m_gpio; }

private:
    std::shared_ptr<hal::IGpio> m_gpio;
};

} // namespace ember::device_manager
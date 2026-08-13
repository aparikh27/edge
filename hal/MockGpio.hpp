#pragma once

#include "gpio.hpp"
#include <string>
#include <vector>

namespace ember::hal{
class MockGPIO : public IGpio {
    public:
    MockGPIO(uint8_t pin_number, PinMode mode = PinMode::Output) : m_pin(pin_number), m_mode(mode),m_state(PinState::Low) {}

    virtual void set_high() {
        set_state(PinState::High);
    }

    virtual void set_low() {
        set_state(PinState::Low);
    }

    [[nodiscard]] virtual PinState read() const {
        return m_state;
    }

    virtual void toggle() {
        set_state(m_state == PinState::High ? PinState::Low : PinState::High);
    }

    private:
        uint8_t m_pin;
        PinMode m_mode;
        PinState m_state;
        std::vector<std::string> m_history;
        void set_state(PinState state) {
            m_state = state;
            std::string m_state_string = "";
            switch (state) {
                case PinState::High:
                    m_state_string = "HIGH";
                    break;
                case PinState::Low:
                    m_state_string = "LOW";
                    break;
            } 
            m_history.push_back("Pin " + std::to_string(m_pin) + "set to " + m_state_string);
        }
};}
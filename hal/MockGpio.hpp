#pragma once

#include "gpio.hpp"
#include <string>
#include <vector>

namespace ember::hal {

class MockGPIO : public IGpio {
public:
    explicit MockGPIO(uint8_t pin_number, PinMode mode = PinMode::Output)
        : m_pin(pin_number), m_mode(mode), m_state(PinState::Low) {}
    ~MockGPIO() override = default;

    void set_high() override {
        set_state(PinState::High);
    }

    void set_low() override {
        set_state(PinState::Low);
    }

    [[nodiscard]] PinState read() const override {
        return m_state;
    }

    void toggle() override {
        set_state(m_state == PinState::High ? PinState::Low : PinState::High);
    }

    [[nodiscard]] const std::vector<std::string>& get_history() const {
        return m_history;
    }

private:
    static constexpr size_t kMaxHistoryEntries = 100;
    uint8_t m_pin;
    PinMode m_mode;
    PinState m_state;
    std::vector<std::string> m_history;

    void set_state(PinState state) {
        m_state = state;
        std::string m_state_string;
        switch (state) {
            case PinState::High:
                m_state_string = "HIGH";
                break;
            case PinState::Low:
                m_state_string = "LOW";
                break;
        } 
        if (m_history.size() >= kMaxHistoryEntries) {
            m_history.erase(m_history.begin());
        }
        m_history.push_back("Pin " + std::to_string(m_pin) + " set to " + m_state_string);
    }
};

} // namespace ember::hal
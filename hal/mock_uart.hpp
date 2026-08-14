#pragma once

#include "uart.hpp"
#include <string>
#include <vector>
#include <algorithm>

namespace ember::hal {

class MockUart : public IUart {
public:
    MockUart() = default;
    ~MockUart() override = default;

    void open(uint32_t baud_rate) override {
        m_baud_rate = baud_rate;
        m_is_open = true;
    }

    void close() override {
        m_is_open = false;
    }

    void write(std::string_view data) override {
        if (!m_is_open) return;
        m_tx_buffer.append(data);
    }

    std::string read(size_t max_bytes) override {
        if (!m_is_open || m_rx_buffer.empty()) {
            return "";
        }
        size_t count = (std::min)(max_bytes, m_rx_buffer.size());
        std::string result = m_rx_buffer.substr(0, count);
        m_rx_buffer.erase(0, count);
        return result;
    }

    [[nodiscard]] bool available() const override {
        return m_is_open && !m_rx_buffer.empty();
    }

    // Helper methods for simulation / testing
    void inject_rx(std::string_view data) {
        m_rx_buffer.append(data);
    }

    [[nodiscard]] const std::string& get_tx_data() const {
        return m_tx_buffer;
    }

    void clear_tx_data() {
        m_tx_buffer.clear();
    }

    [[nodiscard]] bool is_open() const {
        return m_is_open;
    }

    [[nodiscard]] uint32_t get_baud_rate() const {
        return m_baud_rate;
    }

private:
    bool m_is_open{false};
    uint32_t m_baud_rate{0};
    std::string m_tx_buffer;
    std::string m_rx_buffer;
};

} // namespace ember::hal

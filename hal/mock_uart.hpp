#pragma once

#include "uart.hpp"

#include <algorithm>

namespace ember::hal {

class MockUart : public IUart {
public:
    explicit MockUart(uint32_t baud_rate = 115200)
        : m_baud_rate(baud_rate), m_is_open(false) {}

    void open(uint32_t baud_rate) override {
        m_baud_rate = baud_rate;
        m_is_open = true;
    }

    void close() override {
        m_is_open = false;
    }

    // Appends outgoing data to tx buffer so tests can verify what was sent
    void write(std::string_view data) override {
        if (!m_is_open) return;
        m_tx_buffer.append(data);
    }

    // Binary counterpart of write(string_view); required by IUart
    void write(std::span<const uint8_t> data) override {
        if (!m_is_open) return;
        m_tx_buffer.append(reinterpret_cast<const char*>(data.data()), data.size());
    }

    // Reads incoming data out of the rx buffer
    std::string read(size_t max_bytes) override {
        if (!m_is_open || m_rx_buffer.empty()) return "";

        size_t bytes_to_read = std::min(max_bytes, m_rx_buffer.size());
        std::string result = m_rx_buffer.substr(0, bytes_to_read);
        m_rx_buffer.erase(0, bytes_to_read);
        return result;
    }

    [[nodiscard]] bool available() const override {
        return m_is_open && !m_rx_buffer.empty();
    }

    // --- Helper methods for unit testing ---

    [[nodiscard]] bool is_open() const { return m_is_open; }
    [[nodiscard]] uint32_t get_baud_rate() const { return m_baud_rate; }

    // Simulates incoming hardware data arriving on the wire
    void inject_rx(std::string_view incoming_data) {
        m_rx_buffer.append(incoming_data);
    }

    // Inspects everything sent out via write()
    [[nodiscard]] const std::string& get_tx_data() const {
        return m_tx_buffer;
    }

    void clear_tx_data() {
        m_tx_buffer.clear();
    }

private:
    uint32_t m_baud_rate;
    bool m_is_open;
    std::string m_tx_buffer; // Outgoing data written by app
    std::string m_rx_buffer; // Incoming data read by app
};

} // namespace ember::hal
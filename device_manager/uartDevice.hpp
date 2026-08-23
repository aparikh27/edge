#pragma once
#include "../hal/uart.hpp"
#include "device.hpp"
#include <memory>
#include <span>
#include <string_view>
#include <cstdint>

namespace ember::device_manager {
    class UartDevice : public Device {
        public:
            UartDevice(std::string name, std::shared_ptr<hal::IUart> uart_hal, uint32_t baud_rate) 
                : Device(std::move(name)), m_uart(std::move(uart_hal)), m_baud_rate(baud_rate) {}

            bool initialize() override {
                if (!m_uart) return false;
                m_uart->open(m_baud_rate);
                set_state(DeviceState::Initialized);
                return true;
            }

            bool start() override {
                if (get_state() == DeviceState::Initialized || get_state() == DeviceState::Stopped) {
                    if (m_uart) {
                        set_state(DeviceState::Active);
                        return true;
                    }
                    set_state(DeviceState::Error);
                }
                return false;
            }

            void stop() override {
                if (get_state() == DeviceState::Shutdown) return; // Shutdown is terminal
                if (m_uart) {
                    m_uart->close();
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

            // --- SEND MESSAGE API ---

            // Send binary packet data
            void send_bytes(std::span<const uint8_t> data) {
                if (get_state() != DeviceState::Active || !m_uart) {
                    return;
                }
                return m_uart->write(data);
            }

            // Send standard string messages
            void send_message(std::string_view msg) {
                auto bytes = std::span<const uint8_t>(
                    reinterpret_cast<const uint8_t*>(msg.data()), 
                    msg.size()
                );
                return send_bytes(bytes);
            }

            [[nodiscard]] const std::shared_ptr<hal::IUart>& get_uart() const { return m_uart; }

        private:
            std::shared_ptr<hal::IUart> m_uart;
            uint32_t m_baud_rate;
    };
}
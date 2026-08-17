#pragma once
#include "../hal/uart.hpp"
#include "device.hpp"
#include <memory>

namespace ember::device_manager {
    class UartDevice : public Device {
        public:
            UartDevice(std::string name, std::shared_ptr<hal::IUart> uart_hal, uint32_t baud_rate) : Device(std::move(name)), m_uart(std::move(uart_hal)), m_baud_rate(baud_rate) {}

            bool initialize() override {
                if (!m_uart) return false;
                set_state(DeviceState::Initialized);
                return true;

            }
            virtual bool start() {
                if (get_state() == DeviceState::Initialized || get_state() == DeviceState::Stopped) {
                    set_state(DeviceState::Active);
                    return true;
                }
                return false;
                
            }
            virtual void stop() {
                
            }
            virtual void shutdown() {
                
            }
            virtual void reset() {
                
            }
        private:
            std::shared_ptr<hal::IUart> m_uart;
            uint32_t m_baud_rate;

    };
}
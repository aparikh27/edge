#pragma once
#include "../hal/uart.hpp"
#include "device.hpp"
#include <memory>

namespace ember::device_manager {
    class UartDevice : public Device {
        public:
            UartDevice(std::string name, std::shared_ptr<hal::IUart> uart_hal) : Device(std::move(name)), m_uart(std::move(uart_hal)) {}
            
            bool initialize() {

            }
            virtual bool start() {
                
            }
            virtual void stop() {
                
            }
            virtual void shutdown() {
                
            }
            virtual void reset() {
                
            }
        private:
            std::shared_ptr<hal::IUart> m_uart;

    };
}
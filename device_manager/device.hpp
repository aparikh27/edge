#pragma once
#include <vector>
#include <algorithm>    
#include <string>


namespace ember::device_manager 
{
    
enum class DeviceState {
    Uninitialized,
    Initialized,
    Active,
    Stopped,
    Error,
    Shutdown
};

class Device {
    public:
        Device(std::string name) : m_state(DeviceState::Uninitialized), m_name(name) {}
        virtual void initialize() = 0;
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void shutdown() = 0;
        virtual ~Device() = default;
    private:
        DeviceState m_state;
        std::string m_name;
};

class DeviceManager {
    public:
        bool register_device(Device d) {
            m_devices.push_back(d);
            return true;
        }
        bool initialize_device(Device d) {
            auto it = find(m_devices.begin(), m_devices.end(), d);
            if (it == m_devices.end()) {
                return false;
            }
            m_devices[it].initialize();

        }
        bool start_device(Device d) {
            auto it = find(m_devices.begin(), m_devices.end(), d);
            if (it == m_devices.end()) {
                return false;
            }
            m_devices[it].start();
        }
        bool stop_device(Device d) {
            auto it = find(m_devices.begin(), m_devices.end(), d);
            if (it == m_devices.end()) {
                return false;
            }
            m_devices[it].stop();
        }
        bool shutdown_device(Device d) {
            auto it = find(m_devices.begin(), m_devices.end(), d);
            if (it == m_devices.end()) {
                return false;
            }
            m_devices.erase(it);
            return true;

        }

    private:
        std::vector<Device> m_devices;    
};


}
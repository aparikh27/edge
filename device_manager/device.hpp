#pragma once
#include <vector>
#include <algorithm>    


namespace ember::device_manager 
{
    
enum class Device {
    Uninitialized,
    Initialized,
    Active,
    Stopped,
    Error,
    Shutdown
};

class DeviceManager {
    public:
        bool register_device(Device d) {
            m_devices.push_back(d);
            return true;
        }
        bool initialize_device();
        bool start_device();
        bool stop_device();
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
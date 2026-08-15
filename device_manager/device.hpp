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
        bool shutdown_device(Device d);

    private:
        std::vector<Device> m_devices;
    
}


}
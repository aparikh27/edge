#pragma once
#include "device.hpp"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>

class DeviceManager {
public:
    DeviceManager() = default;

    // Prevent copies
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    bool register_device(std::shared_ptr<Device> device) {
        if (!device) return false;
        std::lock_guard<std::mutex> lock(m_mutex);
        
        const auto& name = device->get_name();
        if (m_devices.find(name) != m_devices.end()) {
            return false; // Already registered
        }

        m_devices[name] = device;
        return true;
    }

    bool initialize_device(std::string_view name) {
        auto dev = get_device(name);
        if (!dev) return false;
        return dev->initialize();
    }

    bool start_device(std::string_view name) {
        auto dev = get_device(name);
        if (!dev) return false;
        return dev->start();
    }

    bool stop_device(std::string_view name) {
        auto dev = get_device(name);
        if (!dev) return false;
        dev->stop();
        return true;
    }

    bool shutdown_device(std::string_view name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_devices.find(std::string(name));
        if (it == m_devices.end()) return false;

        it->second->shutdown();
        m_devices.erase(it);
        return true;
    }

    [[nodiscard]] std::shared_ptr<Device> get_device(std::string_view name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_devices.find(std::string(name));
        return (it != m_devices.end()) ? it->second : nullptr;
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<Device>> m_devices;
};

} // namespace ember::device_manager
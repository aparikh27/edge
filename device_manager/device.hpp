#pragma once

#include <string>
#include <string_view>

namespace ember::device_manager {

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
    explicit Device(std::string name) 
        : m_state(DeviceState::Uninitialized), m_name(std::move(name)) {}

    virtual ~Device() = default;

    // Pure virtual lifecycle hooks
    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void shutdown() = 0;
    virtual void reset() = 0;

    // State getters/setters
    [[nodiscard]] const std::string& get_name() const { return m_name; }
    [[nodiscard]] DeviceState get_state() const { return m_state; }

protected:
    void set_state(DeviceState state) { m_state = state; }

private:
    DeviceState m_state;
    std::string m_name;
};

}
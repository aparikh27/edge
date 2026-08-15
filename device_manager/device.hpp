#pragma once


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


}
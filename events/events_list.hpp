#include <string>
#include <cstdint>

namespace ember::events {

// --- Lifecycle & System State ---
struct SystemStartedEvent {};

struct SystemShutdownRequestedEvent {
    std::string reason;
    bool force_immediate{false};
};

struct ModeChangedEvent {
    std::string previous_mode;
    std::string new_mode; // e.g., "INITIALIZATION", "AUTONOMOUS", "FAILSAFE"
};

// --- Hardware & Diagnostics ---
struct BatteryLowEvent {
    int percentage;
    float voltage;
};

struct ThermalWarningEvent {
    std::string component_name;
    float temperature_celsius;
};

struct HardwareFaultEvent {
    uint32_t error_code;
    std::string component_name;
    std::string description;
};

// --- Task & Scheduler Health ---
struct DeadlineMissedEvent {
    std::string task_name;
    uint64_t total_missed_count;
    double execution_time_ms;
};

struct TaskStateChangedEvent {
    std::string task_name;
    std::string new_state; // "RUNNING", "PAUSED", "STOPPED"
};

// --- Communication & Network ---
struct ConnectionLostEvent {
    std::string endpoint_name;
};

struct ConnectionRestoredEvent {
    std::string endpoint_name;
};

} // namespace ember::events
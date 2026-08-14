#include <gtest/gtest.h>
#include "schedule/scheduler.hpp"
#include "messages/coordinator.hpp"
#include "messages/publisher.hpp"
#include "messages/subscriber.hpp"
#include "events/EventBus.hpp"
#include "events/events_list.hpp"
#include "hal/MockGpio.hpp"
#include "hal/mock_uart.hpp"
#include "run/runtime.hpp"
#include "run/time.hpp"

#include <thread>
#include <atomic>
#include <string>

using namespace ember;

TEST(EmberIntegrationTest, EndToEndThermalMonitoringAndControl) {
    // 1. Setup Infrastructure
    Scheduler scheduler;
    messaging::Coordinator coordinator;
    events::EventBus event_bus;
    hal::MockGPIO mock_fan_gpio(13, hal::PinMode::Output);
    hal::MockUart mock_console_uart;
    mock_console_uart.open(115200);

    // Shared simulation state
    std::atomic<float> simulated_temperature{25.0f};
    std::atomic<int> messages_processed_by_controller{0};

    // Pub/Sub setup
    auto controller_subscriber = std::make_shared<messaging::Subscriber>("sensor/temperature");
    coordinator.subscribe(controller_subscriber);

    // Subscribe Controller to EventBus
    event_bus.subscribe<events::ThermalWarningEvent>([&mock_console_uart](const events::ThermalWarningEvent&) {
        std::string warn_msg = "WARNING: Thermal Event\n";
        mock_console_uart.write(warn_msg);
    });

    // 2. Task Definitions
    // SensorTask (10 Hz = 100,000 us)
    messaging::Publisher sensor_publisher(&coordinator);
    scheduler.schedule("SensorTask", std::chrono::microseconds(100000), [&]() {
        float temp = simulated_temperature.load();
        messaging::Message msg{"sensor/temperature", std::chrono::steady_clock::now(), std::to_string(temp)};
        sensor_publisher.push(msg);

        if (temp > 80.0f) {
            events::ThermalWarningEvent warn_event{"CPU_Thermal_Sensor", temp};
            event_bus.publish(warn_event);
        }
    });

    // ControllerTask (50 Hz = 20,000 us)
    scheduler.schedule("ControllerTask", std::chrono::microseconds(20000), [&]() {
        messaging::Message msg;
        while (controller_subscriber->pop(msg)) {
            messages_processed_by_controller.fetch_add(1);
            float temp = std::stof(msg.payload);
            if (temp > 80.0f) {
                mock_fan_gpio.set_high();
            }
        }
    });

    // 3. Execution (Simulate ~200ms of runtime execution)
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() < 100) {
        scheduler.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Inject high temperature (>80°C)
    simulated_temperature.store(85.5f);

    // Run remaining runtime loop
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() < 200) {
        scheduler.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 4. Assertions
    // Assert 1: Messages passed through Pub/Sub queue to ControllerTask
    EXPECT_GT(messages_processed_by_controller.load(), 0);

    // Assert 2: MockGpio recorded state transition to HIGH
    EXPECT_EQ(mock_fan_gpio.read(), hal::PinState::High);

    // Assert 3: MockUart recorded transmission of "WARNING: Thermal Event\n"
    EXPECT_NE(mock_console_uart.get_tx_data().find("WARNING: Thermal Event\n"), std::string::npos);
}

#include "run/log.hpp"
#include "run/time.hpp"
#include "run/runtime.hpp"
#include "events/EventBus.hpp"
#include "events/events_list.hpp"
#include "messages/threads.hpp"
#include "messages/message.hpp"
#include "messages/subscriber.hpp"
#include "messages/publisher.hpp"
#include "messages/coordinator.hpp"
#include "hal/MockGpio.hpp"
#include "hal/mock_uart.hpp"
#include "schedule/scheduler.hpp"

#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace ember;

int main() {
    std::cout << "[TEST] Starting EMBER Framework Integration Verification...\n";

    // 1. Test Logging Kernel
    log::log_message(log::Level::Info, "Testing log_message standard output.");

    // 2. Test EventBus & Nested Emission Safety
    events::EventBus event_bus;
    bool system_started_received = false;
    bool thermal_warning_received = false;

    event_bus.subscribe<events::SystemStartedEvent>([&](const events::SystemStartedEvent&) {
        system_started_received = true;
        // Nested emission during event handler execution (tests deadlock fix)
        events::ThermalWarningEvent warning{"CPU_Core0", 65.5f};
        event_bus.publish(warning);
    });

    event_bus.subscribe<events::ThermalWarningEvent>([&](const events::ThermalWarningEvent& e) {
        thermal_warning_received = true;
        std::cout << "[EVENT] Thermal Warning on " << e.component_name << ": " << e.temperature_celsius << " C\n";
    });

    event_bus.publish(events::SystemStartedEvent{});
    assert(system_started_received);
    assert(thermal_warning_received);
    std::cout << "[PASS] EventBus publish/subscribe & nested emission verified.\n";

    // 3. Test Pub/Sub Messaging & Coordinator
    messaging::Coordinator coordinator;
    auto sub = std::make_shared<messaging::Subscriber>("sensor/temperature");
    coordinator.subscribe(sub);

    messaging::Publisher pub(&coordinator);
    messaging::Message msg{"sensor/temperature", std::chrono::steady_clock::now(), "23.4 C"};
    pub.push(msg);

    messaging::Message recv_msg;
    bool pop_success = sub->pop(recv_msg);
    assert(pop_success);
    assert(recv_msg.payload == "23.4 C");
    sub->alert();
    std::cout << "[PASS] Pub/Sub messaging coordinator verified.\n";

    // 4. Test ThreadSafeQueue Shutdown
    messaging::ThreadSafeQueue<int> queue;
    std::thread consumer([&queue]() {
        int val = 0;
        bool res = queue.wait_and_pop(val);
        assert(!res); // Should return false on shutdown
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.shutdown();
    consumer.join();
    std::cout << "[PASS] ThreadSafeQueue graceful shutdown verified.\n";

    // 5. Test HAL Components (MockGPIO & MockUart)
    hal::MockGPIO gpio_pin(13, hal::PinMode::Output);
    gpio_pin.set_high();
    assert(gpio_pin.read() == hal::PinState::High);
    gpio_pin.toggle();
    assert(gpio_pin.read() == hal::PinState::Low);
    assert(!gpio_pin.get_history().empty());
    std::cout << "[PASS] MockGPIO operations verified.\n";

    hal::MockUart uart;
    uart.open(115200);
    uart.write("HELLO UART");
    assert(uart.get_tx_data() == "HELLO UART");
    uart.inject_rx("RX_DATA");
    assert(uart.available());
    assert(uart.read(10) == "RX_DATA");
    uart.close();
    std::cout << "[PASS] MockUart operations verified.\n";

    // 6. Test Scheduler & Tasks
    schedule::Scheduler scheduler;
    int task_counter = 0;
    scheduler.schedule("HeartbeatTask", std::chrono::microseconds(1000), [&task_counter]() {
        task_counter++;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    scheduler.tick();
    assert(task_counter == 1);
    std::cout << "[PASS] Scheduler & periodic tasks verified.\n";

    // 7. Test Runtime Kernel Execution Loop
    runtime::RuntimeConfig config;
    config.target_frequency_hz = 100;
    runtime::Runtime rt(config);
    
    std::thread rt_thread([&rt]() {
        rt.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(rt.is_running());
    rt.stop();
    rt_thread.join();
    assert(rt.get_state() == runtime::State::Stopped);
    std::cout << "[PASS] Runtime kernel lifecycle verified.\n";

    std::cout << "\n=========================================\n";
    std::cout << "ALL VERIFICATION TESTS PASSED SUCCESSFULLY!\n";
    std::cout << "=========================================\n";
    return 0;
}

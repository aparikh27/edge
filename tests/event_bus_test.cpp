#include <gtest/gtest.h>
#include "events/EventBus.hpp"
#include "events/events_list.hpp"
#include <vector>

using namespace ember::events;

TEST(EventBusTest, TypeIsolation) {
    EventBus bus;
    bool battery_fired = false;
    bool shutdown_fired = false;

    bus.subscribe<BatteryLowEvent>([&battery_fired](const BatteryLowEvent& e) {
        battery_fired = true;
        EXPECT_EQ(e.percentage, 15);
    });

    bus.subscribe<SystemShutdownRequestedEvent>([&shutdown_fired](const SystemShutdownRequestedEvent&) {
        shutdown_fired = true;
    });

    BatteryLowEvent low_batt{15, 3.4f};
    bus.publish(low_batt);

    EXPECT_TRUE(battery_fired);
    EXPECT_FALSE(shutdown_fired);
}

TEST(EventBusTest, MultipleHandlers) {
    EventBus bus;
    std::vector<int> execution_order;

    bus.subscribe<ThermalWarningEvent>([&execution_order](const ThermalWarningEvent&) {
        execution_order.push_back(1);
    });

    bus.subscribe<ThermalWarningEvent>([&execution_order](const ThermalWarningEvent&) {
        execution_order.push_back(2);
    });

    bus.subscribe<ThermalWarningEvent>([&execution_order](const ThermalWarningEvent&) {
        execution_order.push_back(3);
    });

    ThermalWarningEvent warning{"MotorController", 82.5f};
    bus.publish(warning);

    ASSERT_EQ(execution_order.size(), 3);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
    EXPECT_EQ(execution_order[2], 3);
}

TEST(EventBusTest, PayloadAccuracy) {
    EventBus bus;
    DeadlineMissedEvent received_event{};
    bool fired = false;

    bus.subscribe<DeadlineMissedEvent>([&received_event, &fired](const DeadlineMissedEvent& e) {
        received_event = e;
        fired = true;
    });

    DeadlineMissedEvent emitted{"ControlLoopTask", 7, 14.8};
    bus.publish(emitted);

    EXPECT_TRUE(fired);
    EXPECT_EQ(received_event.task_name, "ControlLoopTask");
    EXPECT_EQ(received_event.total_missed_count, 7);
    EXPECT_DOUBLE_EQ(received_event.execution_time_ms, 14.8);
}

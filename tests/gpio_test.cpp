#include <gtest/gtest.h>
#include "hal/MockGpio.hpp"

using namespace ember::hal;

TEST(GpioTest, MockGpioStateTransitionsAndRead) {
    MockGPIO gpio(13, PinMode::Output);

    // Initial state check
    EXPECT_EQ(gpio.read(), PinState::Low);

    gpio.set_high();
    EXPECT_EQ(gpio.read(), PinState::High);

    gpio.set_low();
    EXPECT_EQ(gpio.read(), PinState::Low);

    gpio.toggle();
    EXPECT_EQ(gpio.read(), PinState::High);

    gpio.toggle();
    EXPECT_EQ(gpio.read(), PinState::Low);
}

TEST(GpioTest, HistoryLogRecording) {
    MockGPIO gpio(5, PinMode::Output);

    gpio.set_high();
    gpio.set_low();

    const auto& history = gpio.get_history();
    ASSERT_EQ(history.size(), 2);
    EXPECT_EQ(history[0], "Pin 5 set to HIGH");
    EXPECT_EQ(history[1], "Pin 5 set to LOW");
}

#include <gtest/gtest.h>
#include "hal/mock_uart.hpp"

using namespace ember::hal;

TEST(UartTest, WriteTxBuffer) {
    MockUart uart;
    uart.open(115200);

    EXPECT_TRUE(uart.is_open());
    EXPECT_EQ(uart.get_baud_rate(), 115200);

    uart.write("AT+PING\r\n");
    EXPECT_EQ(uart.get_tx_data(), "AT+PING\r\n");

    uart.write("OK\r\n");
    EXPECT_EQ(uart.get_tx_data(), "AT+PING\r\nOK\r\n");

    uart.clear_tx_data();
    EXPECT_TRUE(uart.get_tx_data().empty());
}

TEST(UartTest, InjectAndReadRxDataSlices) {
    MockUart uart;
    uart.open(9600);

    EXPECT_FALSE(uart.available());

    uart.inject_rx("FRAME_HEADER_DATA_FOOTER");
    EXPECT_TRUE(uart.available());

    // Read first slice of 12 bytes
    std::string slice1 = uart.read(12);
    EXPECT_EQ(slice1, "FRAME_HEADER");
    EXPECT_TRUE(uart.available());

    // Read remaining bytes
    std::string slice2 = uart.read(100);
    EXPECT_EQ(slice2, "_DATA_FOOTER");
    EXPECT_FALSE(uart.available());
}

TEST(UartTest, ClosedStateBehavior) {
    MockUart uart;
    EXPECT_FALSE(uart.is_open());
    EXPECT_FALSE(uart.available());

    uart.write("TEST");
    EXPECT_TRUE(uart.get_tx_data().empty());

    uart.inject_rx("TEST");
    EXPECT_FALSE(uart.available()); // should report unavailable when closed
    EXPECT_EQ(uart.read(10), "");
}

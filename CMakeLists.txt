cmake_minimum_required(VERSION 3.14)
project(EMBER_Framework CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch GoogleTest dependency
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# EMBER framework core library sources
set(EMBER_SOURCES
    run/log.cpp
    run/time.cpp
    run/runtime.cpp
    messages/coordinator.cpp
    messages/publisher.cpp
    messages/subscriber.cpp
    schedule/task.cpp
    schedule/scheduler.cpp
)

add_library(ember_core STATIC ${EMBER_SOURCES})

# Test suite executable
add_executable(ember_test_suite
    tests/thread_safe_queue_test.cpp
    tests/pubsub_test.cpp
    tests/event_bus_test.cpp
    tests/gpio_test.cpp
    tests/uart_test.cpp
    tests/ember_integration_test.cpp
)

target_link_libraries(ember_test_suite PRIVATE ember_core gtest_main gmock)

include(GoogleTest)
gtest_discover_tests(ember_test_suite)

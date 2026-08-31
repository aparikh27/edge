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
    tests/memory_test.cpp
    tests/ember_integration_test.cpp
)

target_link_libraries(ember_test_suite PRIVATE ember_core gtest_main gmock)

include(GoogleTest)
gtest_discover_tests(ember_test_suite)

# ---------------------------------------------------------------------------
# Benchmarking suite
# ---------------------------------------------------------------------------
# Each benchmarks/bench_*.cpp is its own standalone executable using
# std::chrono::high_resolution_clock timing (see benchmarks/bench_framework.hpp).
# Every target accepts an optional --markdown=<path> flag to export a
# GitHub-flavored Markdown results table alongside its console report.

find_package(Threads REQUIRED)

function(add_ember_benchmark target_name source_file)
    add_executable(${target_name} benchmarks/${source_file} ${ARGN})
    target_link_libraries(${target_name} PRIVATE ember_core Threads::Threads)
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
    )
endfunction()

add_ember_benchmark(bench_memory        bench_memory.cpp)
add_ember_benchmark(bench_queue         bench_queue.cpp)
add_ember_benchmark(bench_serialization bench_serialization.cpp)
add_ember_benchmark(bench_pubsub        bench_pubsub.cpp)

# Convenience meta-target: builds every benchmark executable.
add_custom_target(ember_benchmarks
    DEPENDS bench_memory bench_queue bench_serialization bench_pubsub
)

# Runs every benchmark and appends each suite's results into one Markdown
# file (benchmarks/results/BENCHMARKS.md) ready to paste into the README.
set(EMBER_BENCHMARK_RESULTS_MD ${CMAKE_SOURCE_DIR}/benchmarks/results/BENCHMARKS.md)
add_custom_target(run_benchmarks
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/benchmarks/results
    COMMAND ${CMAKE_COMMAND} -E echo "# EMBER Benchmark Results" > ${EMBER_BENCHMARK_RESULTS_MD}
    COMMAND $<TARGET_FILE:bench_memory> --markdown=${EMBER_BENCHMARK_RESULTS_MD}
    COMMAND $<TARGET_FILE:bench_queue> --markdown=${EMBER_BENCHMARK_RESULTS_MD}
    COMMAND $<TARGET_FILE:bench_serialization> --markdown=${EMBER_BENCHMARK_RESULTS_MD}
    COMMAND $<TARGET_FILE:bench_pubsub> --markdown=${EMBER_BENCHMARK_RESULTS_MD}
    DEPENDS ember_benchmarks
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running EMBER benchmark suite and writing benchmarks/results/BENCHMARKS.md"
    VERBATIM
)

# Ember

**Ember** is a lightweight, modern **C++20 embedded runtime framework** designed to provide the software infrastructure required by embedded systems, robotics platforms, IoT devices, and real-time control applications.

Ember provides a modular runtime layer for:

* Task scheduling
* Periodic execution
* Publish/Subscribe messaging
* Thread-safe message passing
* Event-driven communication
* Hardware abstraction
* Device and driver management
* Binary serialization
* Runtime diagnostics and telemetry
* Memory management

The framework is designed around a simple idea:

> **Application code should interact with a consistent runtime and hardware interface without needing to know the details of the underlying platform.**

Ember is inspired by concepts found in **ROS2**, **RTOS architectures**, and embedded **Hardware Abstraction Layers (HALs)**, but is intentionally smaller and designed from the ground up as a learning-focused systems framework.

---

# Table of Contents

* [Overview](#overview)
* [Motivation](#motivation)
* [Architecture](#architecture)
* [Runtime Lifecycle](#runtime-lifecycle)
* [Core Components](#core-components)

  * [Runtime Kernel](#1-runtime-kernel)
  * [Scheduler](#2-scheduler)
  * [Thread-Safe Queue](#3-thread-safe-queue)
  * [Publish/Subscribe Message Bus](#4-publishsubscribe-message-bus)
  * [Event Bus](#5-event-bus)
  * [Hardware Abstraction Layer](#6-hardware-abstraction-layer)
  * [Device Manager](#7-device-manager)
  * [Serialization](#8-serialization)
  * [Diagnostics](#9-diagnostics)
* [System Data Flow](#system-data-flow)
* [Scheduling Architecture](#scheduling-architecture)
* [Messaging Architecture](#messaging-architecture)
* [HAL Architecture](#hal-architecture)
* [Device Management](#device-management)
* [Example Application](#example-application)
* [Repository Structure](#repository-structure)
* [Design Principles](#design-principles)
* [Building](#building)
* [Testing](#testing)
* [Performance and Benchmarking](#performance-and-benchmarking)
* [Roadmap](#roadmap)
* [Future Platform Support](#future-platform-support)

---

# Overview

Ember sits between application-level logic and the underlying operating system or hardware.

```text
┌─────────────────────────────────────────────────────────────┐
│                         APPLICATIONS                        │
│                                                             │
│   Robot Controller     Sensor Hub     IoT Device           │
│   Control Loop         Telemetry      Embedded Service      │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                         Ember RUNTIME                         │
│                                                             │
│  ┌────────────┐  ┌────────────┐  ┌──────────────────────┐  │
│  │ Scheduler  │  │ Message    │  │ Event System         │  │
│  │            │  │ Bus        │  │                      │  │
│  └────────────┘  └────────────┘  └──────────────────────┘  │
│                                                             │
│  ┌────────────┐  ┌────────────┐  ┌──────────────────────┐  │
│  │ Task       │  │ Device     │  │ Serialization        │  │
│  │ System     │  │ Manager    │  │                      │  │
│  └────────────┘  └────────────┘  └──────────────────────┘  │
│                                                             │
│  ┌────────────┐  ┌────────────┐  ┌──────────────────────┐  │
│  │ Thread-Safe│  │ Diagnostics│  │ Runtime / Timing     │  │
│  │ Queues     │  │            │  │                      │  │
│  └────────────┘  └────────────┘  └──────────────────────┘  │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                  HARDWARE ABSTRACTION LAYER                 │
│                                                             │
│        GPIO        UART        SPI        I2C        CAN     │
│                                                             │
└──────────────────────────────┬──────────────────────────────┘
                               │
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
          Linux/POSIX      Raspberry Pi       STM32
```

The upper layers remain independent of the underlying hardware implementation.

For example, an application can interact with:

```cpp
gpio.set_high();
```

without knowing whether the GPIO is backed by:

* a Linux GPIO interface
* Raspberry Pi hardware
* a mock backend
* a future STM32 implementation

---

# Motivation

Modern robotics and embedded systems are composed of many independent software components:

* sensors
* actuators
* controllers
* communication interfaces
* state machines
* telemetry systems
* safety systems
* scheduling logic

Without a runtime layer, these components quickly become tightly coupled.

Ember provides a common execution environment where components can communicate without directly depending on each other.

Instead of:

```text
Sensor ───────────────► Controller
   │                         │
   └─────────────────────────┘
```

Ember encourages:

```text
Sensor
  │
  ▼
Message Bus
  │
  ├──────────► Controller
  │
  ├──────────► Logger
  │
  └──────────► Telemetry
```

This architecture makes components easier to:

* test
* replace
* reuse
* schedule
* monitor
* deploy across different platforms

---

# Architecture

Ember is organized into several layers.

```text
                    APPLICATION LAYER
┌─────────────────────────────────────────────────────────────┐
│ Robot Applications │ IoT Applications │ Sensor Applications│
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
                    RUNTIME SERVICES
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│ Scheduler       Message Bus        Event Bus                │
│                                                             │
│ Task System     Device Manager     Serialization            │
│                                                             │
│ Diagnostics     Memory Management  Configuration            │
│                                                             │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
                   HARDWARE ABSTRACTION
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│ GPIO       UART       SPI       I2C       CAN       PWM     │
│                                                             │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
                       PLATFORM LAYER
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│ Linux/POSIX        Raspberry Pi        STM32 / RTOS         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

The key architectural principle is **dependency inversion**:

> Higher-level application logic depends on stable interfaces rather than concrete hardware implementations.

---

# Runtime Lifecycle

The runtime manages the lifecycle of the entire Ember system.

```text
                    ┌───────────────┐
                    │ Uninitialized │
                    └───────┬───────┘
                            │
                            │ constructor
                            ▼
                    ┌───────────────┐
                    │  Initialized  │
                    └───────┬───────┘
                            │
                            │ run()
                            ▼
                    ┌───────────────┐
                    │    Running    │
                    └───────┬───────┘
                            │
                            │ stop()
                            ▼
                    ┌───────────────┐
                    │   Stopping    │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │    Stopped    │
                    └───────────────┘

                       error
                         │
                         ▼
                    ┌───────────────┐
                    │     Error     │
                    └───────────────┘
```

The runtime provides a controlled lifecycle for initialization, execution, and shutdown.

Example:

```cpp
RuntimeConfig config;

config.target_frequency_hz = 100;

Runtime runtime(config);

runtime.run();
```

The runtime loop periodically executes the scheduler and other registered runtime services.

---

# Core Components

## 1. Runtime Kernel

The Runtime Kernel is the central coordinator of Ember.

Responsibilities include:

* runtime initialization
* lifecycle management
* execution loop
* shutdown
* timing
* service coordination
* error handling

Conceptually:

```text
Runtime
   │
   ├── Scheduler
   ├── Message Bus
   ├── Event Bus
   ├── Device Manager
   └── Diagnostics
```

The kernel provides the execution environment in which the rest of the framework operates.

---

# 2. Scheduler

The scheduler provides periodic and task-based execution.

A task contains information such as:

```text
Task
├── Name
├── Callback
├── Period
├── Priority
├── Next execution time
└── Execution statistics
```

Example:

```cpp
scheduler.schedule(
    "SensorTask",
    10ms,
    sensor_callback
);
```

Multiple tasks can operate at different frequencies:

```text
Sensor Task       100 Hz
Control Task       50 Hz
Telemetry Task     10 Hz
Logging Task        1 Hz
```

The scheduler is responsible for determining when each task becomes eligible for execution.

---

# 3. Thread-Safe Queue

Ember provides a thread-safe queue for communication between concurrent components.

```text
Producer 1 ──────┐
                  │
Producer 2 ──────┼──► Thread-Safe Queue ───► Consumer
                  │
Producer 3 ──────┘
```

The queue provides synchronization around shared data and supports producer/consumer workloads.

Core operations include:

```cpp
queue.push(message);

queue.try_pop(message);

queue.wait_and_pop(message);
```

Synchronization is implemented using standard C++ concurrency primitives such as:

* `std::mutex`
* `std::condition_variable`
* `std::queue`

---

# 4. Publish/Subscribe Message Bus

The Message Bus provides decoupled communication between components.

Publishers do not need to know which components consume their messages.

```text
                    ┌──────────────┐
                    │   Publisher  │
                    └───────┬──────┘
                            │
                            ▼
                    ┌──────────────┐
                    │ Message Bus  │
                    └───────┬──────┘
                            │
                ┌───────────┼───────────┐
                ▼           ▼           ▼
           Subscriber   Subscriber   Subscriber
             Sensor      Logger      Controller
```

For example:

```text
TemperatureSensor
       │
       │ TemperatureMessage
       ▼
   Message Bus
       │
       ├──────► Controller
       │
       ├──────► Logger
       │
       └──────► Telemetry
```

This reduces direct dependencies between system components.

---

# 5. Event Bus

The Event Bus is intended for **system events**, rather than continuous data streams.

Examples include:

```text
BatteryLow
DeviceConnected
DeviceDisconnected
EmergencyStop
RuntimeStarted
RuntimeStopping
CommunicationFailure
```

The distinction is:

```text
MESSAGE BUS

Temperature
IMUData
GPSData
MotorCommand
Telemetry
```

versus:

```text
EVENT BUS

BatteryLow
DeviceLost
EmergencyStop
Shutdown
FaultDetected
```

This allows components to react to important system state changes without tightly coupling themselves to the component that generated the event.

---

# 6. Hardware Abstraction Layer

The HAL separates application logic from platform-specific hardware implementations.

```text
                  Application
                      │
                      ▼
                  GPIO API
                      │
                      ▼
                HAL Interface
                      │
          ┌───────────┼───────────┐
          ▼           ▼           ▼
        Mock        Linux     Raspberry Pi
      Backend      Backend       Backend
```

Current interfaces include:

* GPIO
* UART

The architecture is designed to support additional interfaces such as:

* SPI
* I2C
* CAN
* PWM

without changing higher-level application code.

### GPIO Example

```cpp
GPIO led(13);

led.set_high();

led.set_low();
```

The application interacts with the abstraction rather than the underlying GPIO implementation.

### Mock Hardware

The mock backend allows hardware-dependent code to be tested without physical hardware.

```text
Application
     │
     ▼
    GPIO
     │
     ▼
Mock GPIO Backend
     │
     ▼
Test / Simulation
```

This enables deterministic unit testing and development on systems without embedded hardware.

---

# 7. Device Manager

The Device Manager provides a central interface for registering and managing runtime devices.

```text
                     Device Manager
                           │
          ┌────────────────┼────────────────┐
          ▼                ▼                ▼
     Temperature        IMU            Motor Driver
       Sensor
          │                │                │
          ▼                ▼                ▼
         HAL              HAL              HAL
```

Responsibilities include:

* device registration
* device lookup
* device removal
* device lifecycle
* device enumeration
* driver association

Conceptually:

```cpp
device_manager.register_device(device);

auto device =
    device_manager.get_device("imu");

device_manager.remove_device("imu");
```

The Device Manager allows applications to interact with devices through stable abstractions rather than directly managing individual drivers.

---

# 8. Serialization

Ember provides serialization infrastructure for converting structured messages into byte-oriented representations.

```text
┌──────────────┐
│ C++ Message  │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Serializer   │
└──────┬───────┘
       │
       ▼
┌────────────────┐
│ Binary Payload │
└───────┬────────┘
        │
        ▼
      UART
        │
        ▼
┌────────────────┐
│ Binary Payload │
└───────┬────────┘
        │
        ▼
┌──────────────┐
│ Deserializer │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ C++ Message  │
└──────────────┘
```

A serialized packet can be structured as:

```text
┌────────┬────────┬────────┬────────────┬──────────┐
│ Header │ Type   │ Length │  Payload   │ Checksum │
└────────┴────────┴────────┴────────────┴──────────┘
```

This provides the foundation for communication between:

* runtime components
* devices
* processes
* external embedded systems

and eventually other physical platforms.

---

# 9. Diagnostics

The Diagnostics subsystem provides runtime observability.

The goal is to answer:

> What is the runtime doing, and how well is it doing it?

Potential metrics include:

```text
Runtime
├── Uptime
├── Runtime state
└── Tick frequency

Scheduler
├── Task count
├── Execution count
├── Execution time
├── Scheduling latency
└── Missed deadlines

Messaging
├── Messages published
├── Messages delivered
├── Dropped messages
└── Queue depth

Devices
├── Registered devices
├── Active devices
└── Device errors
```

Example:

```text
========================================
             Ember DIAGNOSTICS
========================================

Runtime
  State:             RUNNING
  Uptime:            42.31 s
  Tick Rate:         100 Hz

Scheduler
  Tasks:             4
  Missed Deadlines:  0

SensorTask
  Executions:        4231
  Avg Runtime:       0.31 ms
  Max Runtime:       0.84 ms

Message Bus
  Published:         12,483
  Delivered:         12,481
  Dropped:           2

Devices
  Registered:        3
  Active:            3

========================================
```

---

# System Data Flow

A typical Ember application can combine multiple runtime subsystems:

```text
                     SENSOR
                       │
                       ▼
                ┌──────────────┐
                │ Sensor Task  │
                └──────┬───────┘
                       │
                       │ SensorData
                       ▼
                ┌──────────────┐
                │ Message Bus  │
                └──────┬───────┘
                       │
             ┌─────────┼─────────┐
             ▼         ▼         ▼
        Controller   Logger   Telemetry
             │
             │ ControlCommand
             ▼
        ┌──────────────┐
        │ Device Layer │
        └──────┬───────┘
               │
               ▼
             HAL
               │
               ▼
          Physical Device
```

System failures and state changes can travel through the Event Bus:

```text
Device Failure
      │
      ▼
 Device Manager
      │
      ▼
  Event Bus
      │
      ├────────► Safety Controller
      ├────────► Logger
      └────────► Telemetry
```

---

# Scheduling Architecture

Ember uses periodic tasks as a fundamental execution primitive.

```text
                    Scheduler
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
      SensorTask    ControlTask   LoggerTask
        100 Hz         50 Hz         10 Hz
          │             │             │
          ▼             ▼             ▼
       Sensor        Controller     Logging
```

The scheduler determines whether a task is ready to execute based on its configured execution period.

A future scheduler architecture can support:

```text
Priority
   │
   ▼
Task Ready Queue
   │
   ▼
Worker / Execution Context
   │
   ▼
Task Callback
```

This provides a foundation for more advanced scheduling policies and real-time behavior.

---

# Messaging Architecture

Ember separates message transport from application logic.

```text
                Publisher
                    │
                    ▼
             ┌────────────┐
             │ Message    │
             │ Bus        │
             └─────┬──────┘
                   │
                   ▼
          Thread-Safe Queue
                   │
          ┌────────┼────────┐
          ▼        ▼        ▼
       Consumer  Consumer  Consumer
```

This architecture allows multiple components to consume the same data without requiring the publisher to know their identities.

For example:

```text
IMU
 │
 └── IMUData
       │
       ▼
   Message Bus
       │
       ├──► Localization
       ├──► Stabilization
       ├──► Logger
       └──► Telemetry
```

---

# HAL Architecture

The HAL follows an interface/backend model.

```text
                  Application
                      │
                      ▼
              Hardware Interface
                      │
          ┌───────────┴───────────┐
          ▼                       ▼
     Mock Backend             Real Backend
          │                       │
          ▼                       ▼
       Testing              Physical Hardware
```

This makes it possible to develop and test most of the framework without requiring physical hardware.

A future platform abstraction could look like:

```text
                     Ember HAL
                        │
        ┌───────────────┼────────────────┐
        ▼               ▼                ▼
      POSIX          Raspberry Pi      STM32
        │               │                │
      Linux          Linux GPIO      Bare Metal /
                                      RTOS
```

---

# Device Management

Devices are managed independently from application logic.

```text
Application
     │
     ▼
Device Manager
     │
     ├── Register
     ├── Initialize
     ├── Start
     ├── Stop
     ├── Remove
     └── Query
     │
     ▼
Device Interface
     │
     ▼
Driver
     │
     ▼
HAL
     │
     ▼
Hardware
```

This creates a clear separation between:

```text
Application Logic
        ↓
Device Abstraction
        ↓
Driver
        ↓
Hardware Interface
        ↓
Hardware
```

---

# Example Application

The primary goal of Ember is not to provide isolated libraries.

The individual components should work together as a complete runtime.

A representative application is a sensor-driven controller:

```text
                 ┌─────────────────┐
                 │ Ember Runtime    │
                 └────────┬────────┘
                          │
                    ┌─────▼─────┐
                    │ Scheduler │
                    └─────┬─────┘
                          │
                    Sensor Task
                          │
                          ▼
                    Sensor Data
                          │
                          ▼
                    Message Bus
                          │
                          ▼
                    Controller
                          │
                    Control Event
                          │
                          ▼
                      Event Bus
                          │
                          ▼
                   Device Manager
                          │
                          ▼
                       GPIO HAL
                          │
                          ▼
                    Hardware
```

This demonstrates how Ember's individual subsystems interact within a single application.

---

# Repository Structure

```text
Ember/
│
├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── include/
│   └── Ember/
│       ├── runtime/
│       ├── scheduler/
│       ├── messaging/
│       ├── events/
│       ├── hal/
│       ├── devices/
│       ├── serialization/
│       ├── diagnostics/
│       ├── memory/
│       └── utilities/
│
├── src/
│   ├── runtime/
│   ├── scheduler/
│   ├── messaging/
│   ├── events/
│   ├── hal/
│   ├── devices/
│   ├── serialization/
│   ├── diagnostics/
│   ├── memory/
│   └── utilities/
│
├── tests/
│   ├── runtime/
│   ├── scheduler/
│   ├── messaging/
│   ├── events/
│   ├── hal/
│   ├── devices/
│   └── serialization/
│
├── examples/
│   ├── basic_runtime/
│   ├── scheduler_demo/
│   ├── messaging_demo/
│   └── end_to_end/
│
├── benchmarks/
│
├── docs/
│   ├── architecture/
│   ├── design/
│   └── api/
│
├── configs/
│
└── tools/
```

---

# Design Principles

Ember is built around several systems-engineering principles.

### 1. Modularity

Each subsystem should have a clearly defined responsibility.

```text
Scheduler ≠ Message Bus ≠ HAL ≠ Device Manager
```

---

### 2. Low Coupling

Components communicate through interfaces and messaging rather than direct dependencies.

---

### 3. Testability

Hardware-dependent code should be replaceable with mock implementations.

---

### 4. Deterministic Behavior

Timing-sensitive operations should have predictable execution characteristics.

---

### 5. Zero-Cost Abstractions

Where possible, C++ abstractions should not introduce runtime overhead beyond what is necessary for the abstraction itself.

---

### 6. Platform Independence

Application code should remain largely independent of the underlying operating system and hardware.

---

### 7. Explicit Resource Management

Embedded systems have constrained resources.

Ember therefore emphasizes explicit control over:

* memory
* threads
* queues
* timing
* device lifecycle

---

# Building

Ember is written in **C++20** and uses **CMake** as its build system.

Clone the repository:

```bash
git clone <repository-url>
cd Ember
```

Configure:

```bash
cmake -B build
```

Build:

```bash
cmake --build build
```

Run tests:

```bash
ctest --test-dir build
```

Run an example:

```bash
./build/examples/end_to_end
```

---

# Testing

Testing is an important part of the framework.

Subsystems should be independently testable:

```text
Runtime
Scheduler
Queue
Message Bus
Event Bus
GPIO
UART
Device Manager
Serialization
Diagnostics
```

Example scheduler tests include:

```text
✓ Task executes at configured period
✓ Tasks do not execute before their deadline
✓ Multiple periodic tasks execute independently
✓ Scheduler handles task cancellation
✓ Scheduler maintains expected execution frequency
```

Messaging tests include:

```text
✓ Messages are delivered to subscribers
✓ Multiple subscribers receive messages
✓ Unknown topics are handled safely
✓ Concurrent producers are supported
✓ Queue operations are thread-safe
```

Hardware tests can use mock backends:

```text
Application
     │
     ▼
GPIO Interface
     │
     ▼
Mock GPIO
     │
     ▼
GoogleTest
```

This allows hardware-related behavior to be tested on a development machine.

---

# Performance and Benchmarking

A major goal of Ember is to understand not only whether the system works, but **how efficiently it works**.

Benchmarks will measure:

### Scheduler

* task dispatch latency
* execution frequency accuracy
* scheduling overhead
* missed deadlines

### Messaging

* messages per second
* message delivery latency
* queue throughput
* concurrent producer/consumer performance

### Serialization

* serialization latency
* deserialization latency
* throughput
* serialized message size

### Memory

* allocation latency
* deallocation latency
* memory reuse
* fragmentation

Performance results will be documented as the framework evolves.

---

# Roadmap

## Runtime

* [x] Runtime lifecycle
* [x] Logging
* [x] Timing utilities
* [x] Runtime execution loop

## Scheduling

* [x] Task abstraction
* [x] Periodic scheduler
* [x] Task execution
* [x] Advanced priority scheduling
* [x] Scheduler latency metrics
* [x] Multi-threaded execution

## Messaging

* [x] Thread-safe queue
* [x] Publish/Subscribe message bus
* [x] Event bus
* [x] Serialization
* [x] Advanced message routing
* [x] Inter-process communication

## Hardware

* [x] GPIO abstraction
* [x] Mock GPIO backend
* [x] UART abstraction
* [ ] SPI
* [ ] I2C
* [ ] PWM
* [ ] CAN
* [ ] Raspberry Pi hardware backend
* [ ] STM32 backend

## Devices

* [x] Device abstraction
* [x] Device Manager
* [x] Device registration
* [x] Driver lifecycle management
* [x] Device discovery

## Observability

* [x] Runtime diagnostics
* [x] Task statistics
* [x] Queue metrics
* [x] Message throughput metrics
* [x] Runtime telemetry
* [x] Performance benchmarks

## Memory

* [x] Memory Pool
* [x] Object Pool
* [x] Buffer Pool
* [x] Allocation benchmarks

---

# Future Platform Support

The architecture is designed to eventually support multiple execution environments.

```text
                         Ember
                          │
              ┌───────────┼───────────┐
              │           │           │
              ▼           ▼           ▼
           Linux      Raspberry Pi   STM32
              │           │           │
              ▼           ▼           ▼
           POSIX       Linux GPIO    Bare Metal
                                    / FreeRTOS
```

The goal is for application code to remain largely unchanged when moving between platforms.

For example:

```cpp
GPIO led(13);

led.set_high();
```

should remain the same regardless of whether the underlying implementation is:

```text
Linux
Raspberry Pi
STM32
Mock
```

---

# Why Ember?

Ember is an exploration of the infrastructure underneath modern embedded and robotics applications.

Rather than building another application on top of existing frameworks, Ember focuses on understanding and implementing the systems that make those applications possible:

```text
                    AI / ROBOT APPLICATIONS
                              │
                              ▼
                    ┌─────────────────┐
                    │     Ember        │
                    │                 │
                    │ Scheduling      │
                    │ Messaging       │
                    │ Devices         │
                    │ Hardware        │
                    │ Memory          │
                    │ Diagnostics     │
                    └────────┬────────┘
                             │
                             ▼
                         HARDWARE
```

The project is an ongoing exploration of **systems programming, embedded architecture, concurrency, hardware abstraction, and runtime design in modern C++**.

---

# Ember Benchmarks and Metrics

=== EMBER Memory Management Benchmarks ===

-- Memory Management --
Benchmark                            Min(us)   P50(us)  Mean(us)   P95(us)   P99(us)   Max(us)   Throughput
------------------------------------------------------------------------------------------------------------------------
MemoryPool: alloc+dealloc              0.000     0.100     0.068     0.100     0.100    33.300   4.29 M ops/sec
Heap (::operator new/delete): alloc+dealloc     0.000     0.100     0.072     0.100     0.100   177.500   4.43 M ops/sec
MemoryPool: alloc+dealloc (heap fragmented)     0.000     0.100     0.066     0.100     0.100    40.200   4.76 M ops/sec
Heap: alloc+dealloc (fragmented)       0.000     0.100     0.057     0.100     0.100    16.000   5.24 M ops/sec

  * **Determinism & WCET:** The fixed-block `MemoryPool` capped worst-case execution time (WCET) at **33.3 µs** under normal conditions and **40.2 µs** under fragmentation, compared to **177.5 µs** for system `malloc`/`new`. This provides a $4.4\times$ reduction in peak tail latency.
  * **Throughput:** Maintains a steady **4.29M–4.76M ops/sec**, providing predictable $O(1)$ allocation times without heap fragmentation risk.

=== EMBER Pub/Sub Message Bus Benchmarks ===

-- Pub/Sub EventBus (sync) --
Benchmark                            Min(us)   P50(us)  Mean(us)   P95(us)   P99(us)   Max(us)   Throughput
------------------------------------------------------------------------------------------------------------------------
1 subscriber(s)                        0.100     0.200     0.184     0.200     0.300     9.300   3.81 M handler-calls/sec
5 subscriber(s)                        0.200     0.400     0.386     0.500     0.600    31.700   7.71 M handler-calls/sec
10 subscriber(s)                       0.200     0.600     0.596     0.800     1.000    18.900   9.58 M handler-calls/sec

-- Pub/Sub Coordinator (async) --
Benchmark                            Min(us)   P50(us)  Mean(us)   P95(us)   P99(us)   Max(us)   Throughput
------------------------------------------------------------------------------------------------------------------------
1 subscriber(s)                        0.800     3.900    10.153    49.705    60.800    82.500   1.04 M deliveries/sec
5 subscriber(s)                        1.300     7.300     7.735    10.700    18.200   137.100   645.16 K deliveries/sec
10 subscriber(s)                       3.000    10.200    10.751    16.400    18.300   251.300   699.06 K deliveries/sec

  * **Synchronous `EventBus`:** Achieves sub-microsecond median latencies (**0.20 µs–0.60 µs**) and scales up to **9.58M handler calls/sec** across 10 subscribers with negligible CPU overhead.
  * **Asynchronous `Coordinator`:** Decouples thread execution boundaries with median delivery times between **3.90 µs and 10.20 µs**, maintaining **645K–1.04M deliveries/sec**. The latency tradeoff safely isolates high-frequency execution loops from slower consumers.

=== EMBER ThreadSafeQueue Benchmarks ===

-- ThreadSafeQueue / Concurrency --
Benchmark                            Min(us)   P50(us)  Mean(us)   P95(us)   P99(us)   Max(us)   Throughput
------------------------------------------------------------------------------------------------------------------------
SPSC push+pop                          7.400 11314.650 10889.946 18554.700 19147.300 19315.000   7.77 M ops/sec
MPMC push+pop                         26.700 14135.850 12965.747 20757.900 21295.004 21480.900   4.83 M ops/sec

  * **SPSC Throughput:** Reaches **7.77M ops/sec** under single-producer/single-consumer conditions.
  * **MPMC Under Contention:** Retains **4.83M ops/sec** under multi-threaded contention. The higher median latencies (~11–14 ms) reflect full thread-blocking synchronization wait times, while maintaining high total queue throughput.

=== EMBER Serialization Engine Benchmarks ===

-- Serialization Engine --
Benchmark                            Min(us)   P50(us)  Mean(us)   P95(us)   P99(us)   Max(us)   Throughput
------------------------------------------------------------------------------------------------------------------------
pack() 16B payload                     0.200     0.300     0.294     0.500     0.600    39.800   36.62 M bytes/sec (payload)
pack() 64B payload                     0.400     0.500     0.505     0.600     0.700    19.300   100.18 M bytes/sec (payload)
pack() 256B payload                    1.300     1.400     1.441     1.800     2.000    35.600   158.43 M bytes/sec (payload)
pack() 1024B payload                   4.800     4.900     5.105     5.100    10.600   696.500   188.52 M bytes/sec (payload)
unpack+verify() 16B payload            0.100     0.200     0.185     0.300     0.500    20.500   71.96 M bytes/sec (wire)
unpack+verify() 64B payload            0.300     0.400     0.391     0.400     0.400    34.100   136.30 M bytes/sec (wire)
unpack+verify() 256B payload           1.200     1.300     1.283     1.300     1.300    67.300   181.03 M bytes/sec (wire)
unpack+verify() 1024B payload          4.700     4.800     4.899     4.900     6.201    39.800   197.95 M bytes/sec (wire)
calculate_fletcher16() 1024B           4.600     4.700     4.982     5.000    13.500   688.600   193.12 M bytes/sec
calculate_fletcher16() 4096B          18.700    18.800    19.846    27.000    36.401   713.000   196.27 M bytes/sec
calculate_fletcher16() 65536B        299.100   301.300   317.904   386.300   455.100  5143.400   198.15 M bytes/sec

  * **Payload Scaling:** Scaling payload sizes from 16B to 1024B increases bandwidth efficiently, capping out near memory bus limits at **~188.5 MB/s** for packing and **~198.0 MB/s** for unpacking/verification.
  * **Checksum Bandwidth:** The raw Fletcher-16 implementation saturates at **~198.15 MB/s** for large buffers (64 KiB), ensuring minimal checksum overhead on packet serialization pipelines.

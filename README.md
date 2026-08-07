# Edge


**Edge** is a lightweight, modern C++20 embedded runtime framework designed to provide core middleware services, hardware abstraction, task scheduling, inter-process messaging, and device management for embedded systems and robotics applications.

Think of EMBER as a modular, lightweight cross between **ROS2**, a **Real-Time Operating System (RTOS)**, and a modern **Hardware Abstraction Layer (HAL)**—built with zero external heavy dependencies and zero-overhead C++ abstractions.

---

## Architecture Overview

```text
                     Applications
       (Robot / IoT / Sensor Hub / Control Loops)
                          │
                          ▼
======================================================
               EMBER Runtime Framework
======================================================
  Message Bus (Pub/Sub)   │  Periodic Scheduler
  Thread-Safe Queues      │  Parameter Server
  Event System            │  Device & Driver Manager
  Memory Pools            │  Diagnostics & Telemetry
======================================================
               Hardware Abstraction (HAL)
     [GPIO]   [UART]   [SPI]   [I2C]   [CAN]   [PWM]
======================================================
    Linux / POSIX   │   Raspberry Pi   │   STM32 (Baremetal/RTOS)
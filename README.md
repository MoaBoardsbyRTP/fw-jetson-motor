# fw-jetson-motor

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32%20C3-orange)](https://platformio.org/)
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-v10.4-blue)](https://www.freertos.org/)

> **Moa ESC Motor Controller** - A professional-grade, FreeRTOS-based Electronic Speed Controller for electric skateboards and similar applications, running on ESP32-C3 with advanced safety features and telemetry.

---

## Overview

This repository contains the firmware for the **Moa ESC Controller**, a sophisticated motor control system designed for electric skateboards and personal electric vehicles. Built on the ESP32-C3 platform with FreeRTOS, it provides real-time sensor monitoring, state-machine-based safety management, and responsive user input handling.

### Key Features

- **State Machine Architecture** - 6 distinct states with proper safety transitions
- **Multi-Sensor Monitoring** - Temperature (DS18B20), current (ACS759-200B), battery voltage (ADC)
- **Safety-First Design** - Overheat, overcurrent, and low battery protection
- **Interrupt-Driven Input** - 5 hardware buttons via MCP23018 I/O expander
- **Ramped Throttle Control** - Smooth ESC transitions (configurable rate)
- **LED Visual Feedback** - Board lock/unlock status, warning indicators
- **Flash-Based Logging** - Event logging with JSON export capability
- **Real-Time Telemetry** - Thread-safe stats aggregation

---

## Hardware Platform

| Component | Specification |
|-----------|--------------|
| **MCU** | ESP32-C3 (DFRobot Beetle) |
| **I/O Expander** | MCP23018 (I2C) - 5 buttons, 5 LEDs |
| **Temperature** | DS18B20 (1-Wire, non-blocking) |
| **Current Sensor** | ACS759-200B Hall Effect (bidirectional) |
| **Battery Monitor** | ADC with voltage divider |
| **ESC Output** | PWM signal |
| **Storage** | LittleFS flash logging |

---

## Project Structure

```
fw-jetson-motor/
├── jetsonToESCControl/           # Main firmware project
│   ├── include/                  # Header files
│   │   ├── Devices/             # Hardware abstraction classes
│   │   ├── Helpers/             # System services & managers
│   │   ├── StateMachine/        # State pattern implementation
│   │   └── Tasks/               # FreeRTOS task declarations
│   ├── src/                     # Source implementation
│   │   ├── Devices/             # Sensor & I/O implementations
│   │   ├── Helpers/             # Manager implementations
│   │   ├── StateMachine/        # State logic implementations
│   │   ├── Tasks/               # FreeRTOS task implementations
│   │   └── main.cpp             # Application entry point
│   ├── test/                    # Test suite
│   ├── platformio.ini           # PlatformIO configuration
│   ├── ARCHITECTURE.md          # Detailed architecture documentation
│   └── PLAN.md                  # Development plan & roadmap
├── README.md                    # This file
└── LICENSE                      # License file
```

---

## State Machine

The controller implements a robust state machine with 6 states and cross-safety transitions:

```
                    ┌─────────────┐
                    │  InitState  │ (Board Locked)
                    └──────┬──────┘
                           │ long press STOP (1s)
                           ▼
                    ┌─────────────┐
         ┌─────────│  IdleState  │◄───────────────────┐
         │         │ (Unlocked)  │                    │
         │         └──────┬──────┘                    │
         │                │ throttle button           │
         │                ▼                         │
         │         ┌─────────────┐  safety event  │
         │         │ SurfingState│ ─────────► Error States
         │         └──────┬──────┘                  │
         │                │ STOP press             │
         └────────────────┘                         │
                                                   │
    Error States:                                  │
    ┌──────────────────────────────────────────────┘
    │
    ├─ OverHeatingState    ── temp below ──► Idle
    ├─ OverCurrentState    ── current normal ──► Idle
    └─ BatteryLowState     ── batt med/high ──► Idle

    (All error states: long press STOP → Init)
```

---

## Quick Start

### Prerequisites

- [PlatformIO](https://platformio.org/) IDE or CLI
- ESP32-C3 development board
- MCP23018 I/O expander (for buttons and LEDs)
- DS18B20 temperature sensor
- ACS759-200B current sensor
- ESC with PWM input

### Building

```bash
cd jetsonToESCControl
platformio run
```

### Flashing

```bash
platformio run --target upload
```

### Monitor Serial Output

```bash
platformio device monitor --baud 115200
```

---

## Architecture

The firmware uses a layered architecture with FreeRTOS task separation:

### Task Structure

| Task | Priority | Period | Responsibility |
|------|----------|--------|----------------|
| **SensorTask** | High | 50ms | Temperature, battery, current monitoring |
| **IOTask** | Medium | 20ms | Button processing, LED updates, ESC ramp |
| **ControlTask** | Medium | Event-driven | State machine event processing |
| **StatsTask** | Low | Event-driven | Telemetry aggregation |

### Event System

All communication uses a unified `ControlCommand` event structure via FreeRTOS queues:

```cpp
struct ControlCommand {
    int controlType;   // Producer: Timer, Temp, Batt, Current, Button
    int commandType;   // Event type
    int value;         // Data value
};
```

### Key Design Principles

1. **Thread-Safe I2C** - MCP23018 access protected by mutex
2. **Interrupt-Driven Buttons** - MCP23018 INTA → ESP32 GPIO2
3. **Hardware Reset** - Dedicated GPIO10 for I2C recovery
4. **Non-Blocking Sensors** - DS18B20 async conversion
5. **Ramped Throttle** - Smooth ESC transitions (100%/s default)

---

## Documentation

- **[ARCHITECTURE.md](jetsonToESCControl/ARCHITECTURE.md)** - Complete system architecture and design details
- **[PLAN.md](jetsonToESCControl/PLAN.md)** - Development roadmap and implementation status
- **[UART_CLI.md](jetsonToESCControl/UART_CLI.md)** - Serial command interface documentation
- **[CONFIG_MANAGER_PLAN.md](jetsonToESCControl/CONFIG_MANAGER_PLAN.md)** - Configuration system design

---

## Safety Features

- **Overheat Protection** - Automatic motor stop at 60°C, recovery at 50°C
- **Overcurrent Protection** - Cutoff at 180A with auto-recovery
- **Low Battery Protection** - Stops motor below 11.1V
- **Emergency Stop** - Long-press STOP button (1s) locks board from any state
- **Watchdog Behavior** - Throttle timeout, stuck button recovery

---

## LED Indicators

| LED | Function | States |
|-----|----------|--------|
| **Overcurrent** | Board lock status | Solid ON (locked), OFF (unlocked), Blink (warning) |
| **Temperature** | Overheat warning | Blink when overheating |
| **Battery Low** | Battery level | ON when battery low |
| **Battery Medium** | Battery level | ON when medium charge |
| **Battery High** | Battery level | ON when high charge |

---

## Recent Updates

- **2026-02-18** - Fixed LED state initialization in error states (OverHeatingState, OverCurrentState, BatteryLowState)
- **2026-02-12** - Full state machine implementation with cross-safety transitions
- **2026-02-11** - Non-blocking DS18B20, ESC ramping integration
- **2026-02-06** - Interrupt-driven buttons with hardware reset support

---

## Contributing

This is a proprietary project. Contributions are managed through the internal development team.

---

## License

Proprietary - All rights reserved.

---

## Contact

For technical questions or issues, contact the development team.

---

*Built with PlatformIO and FreeRTOS for ESP32-C3*

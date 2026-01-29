# Moa ESC Controller - Architecture Plan

## Overview

FreeRTOS-based architecture using the State Pattern for managing ESC control, sensor monitoring, and user input. Designed for extensibility to support future BLE control and WiFi configuration.

---

## Target Hardware

- **MCU:** ESP32-C3 (DFRobot Beetle)
- **I/O Expander:** MCP23018 (I2C) - buttons and LEDs
- **Sensors:** Temperature (DS18B20), Current, Battery voltage
- **Output:** ESC via PWM

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        Event Queue                              │
│                  (ControlCommand + EventType)                   │
└───────────────────────────┬─────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        ▼                   ▼                   ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│  IOTask       │   │  SensorTask   │   │  [Future]     │
│  (MCP23018)   │   │  (Temp/Curr)  │   │  BLE/Web/etc  │
└───────────────┘   └───────────────┘   └───────────────┘
                            │
                            ▼
                ┌───────────────────────┐
                │     ControlTask       │
                │   MoaStateMachine     │
                │   ESCController       │
                └───────────────────────┘
```

---

## State Machine

### States

| State | Description |
|-------|-------------|
| **InitState** | Boot sequence, hardware init |
| **IdleState** | Waiting for user input, ESC stopped |
| **SurfingState** | Normal operation, ESC active |
| **OverHeatingState** | Temperature limit exceeded, ESC reduced/stopped |
| **OverCurrentState** | Current limit exceeded, ESC stopped |
| **BatteryLowState** | Battery critical, ESC stopped |
| **ConfigState** | [Future] WiFi AP + webserver for configuration |

### Events

| EventType | Source | Description |
|-----------|--------|-------------|
| `EVT_BUTTON_CLICK` | IOTask, BLE | User button press |
| `EVT_OVERCURRENT` | SensorTask | Current threshold crossed |
| `EVT_TEMPERATURE_LIMIT` | SensorTask | Temperature threshold crossed |
| `EVT_BATTERY_LIMIT` | SensorTask | Battery threshold crossed |
| `EVT_THROTTLE_REQUEST` | [Future] BLE/Jetson | External throttle command |
| `EVT_PARAM_CHANGE` | [Future] Web | Configuration parameter changed |

---

## FreeRTOS Tasks

| Task | Priority | Period | Responsibility |
|------|----------|--------|----------------|
| **SensorTask** | High | 50ms | Read ADC (current, battery), DS18B20 (temp), push threshold events |
| **IOTask** | Medium | 20ms | Poll MCP23018 buttons, debounce, detect clicks, update LEDs |
| **ControlTask** | Medium | Event-driven | Process event queue, run StateMachine, update ESCController ramp |
| **BLETask** | Medium | Event-driven | [Future] GATT server, BLE commands → events |
| **WebTask** | Low | N/A | [Future] Config webserver (only in ConfigState) |

---

## Synchronization

- **Event Queue:** Single FreeRTOS queue for all events → ControlTask
- **I2C Mutex:** Protect I2C bus if multiple tasks access (MCP23018 + sensors)
- **Config Mutex:** [Future] Protect shared config struct for web access

---

## Implementation Phases

### Phase 1: Core (V1)

- [ ] Define `ControlCommand` struct and `EventType` enum
- [ ] Create FreeRTOS task stubs and event queue
- [ ] Implement SensorTask (temperature, current, battery reads)
- [ ] Implement IOTask (MCP23018 button polling, LED control)
- [ ] Implement ControlTask (event processing, StateMachine integration)
- [ ] Wire ESCController to StateMachine (ramp control in SurfingState)
- [ ] Implement state transitions and error states

### Phase 2: Refinement

- [ ] Button debounce and click pattern detection (single, double, long)
- [ ] LED feedback patterns per state
- [ ] Tunable thresholds (hardcoded initially)
- [ ] Serial debug output / telemetry

### Phase 3: BLE Control (Future)

- [ ] Add BLETask with GATT server
- [ ] Define BLE characteristics for button simulation
- [ ] Map BLE commands to `EVT_BUTTON_CLICK` events
- [ ] Optional: throttle control characteristic

### Phase 4: WiFi Configuration (Future)

- [ ] Add ConfigState to StateMachine
- [ ] Entry via long-press or boot combo
- [ ] Start WiFi AP, run AsyncWebServer
- [ ] Serve config page (params, thresholds)
- [ ] Store config in NVS
- [ ] Reboot to normal operation on save

---

## File Structure

```
jetsonToESCControl/
├── include/
│   ├── StateMachine/
│   │   ├── MoaState.h
│   │   ├── MoaStateMachine.h
│   │   ├── InitState.h
│   │   ├── IdleState.h
│   │   ├── SurfingState.h
│   │   ├── OverHeatingState.h
│   │   ├── OverCurrentState.h
│   │   └── BatteryLowState.h
│   ├── ControlCommand.h
│   ├── EventTypes.h          [TODO]
│   ├── Tasks.h               [TODO]
│   └── Config.h              [TODO]
├── src/
│   ├── StateMachine/
│   │   ├── MoaStateMachine.cpp
│   │   ├── InitState.cpp
│   │   ├── IdleState.cpp
│   │   ├── SurfingState.cpp
│   │   ├── OverHeatingState.cpp
│   │   ├── OverCurrentState.cpp
│   │   └── BatteryLowState.cpp
│   ├── Tasks/                [TODO]
│   │   ├── SensorTask.cpp
│   │   ├── IOTask.cpp
│   │   └── ControlTask.cpp
│   └── main.cpp
├── lib/
│   ├── ESCController/
│   └── MCP23018/
└── platformio.ini
```

---

## Key Design Principles

1. **State machine is source-agnostic** — doesn't know where events come from
2. **Single task owns state machine** — no mutex needed for state transitions
3. **Event queue decouples producers/consumers** — easy to add new input sources
4. **ConfigState is mutually exclusive** — WiFi/web only runs in dedicated mode
5. **I2C protected by mutex** — safe concurrent access from multiple tasks

---

## Notes

- ESP32-C3 is single-core; FreeRTOS provides preemptive multitasking, not parallelism
- BLE 5.0 only (no Classic Bluetooth)
- WiFi + BLE coexistence possible but not needed if ConfigState is exclusive
- Keep webserver minimal to conserve RAM

---

*Last updated: 2026-01-29*

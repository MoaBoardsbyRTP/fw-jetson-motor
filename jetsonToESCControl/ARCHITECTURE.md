# Moa ESC Controller - Architecture Plan

## Overview

FreeRTOS-based architecture using the State Pattern for managing ESC control, sensor monitoring, and user input. Designed for extensibility to support future BLE control and WiFi configuration.

**Implementation Status:** Phase 1 complete - Architecture implemented, project reorganized to match RTPBuit pattern. Ready for state machine wiring.

---

## Target Hardware

- **MCU:** ESP32-C3 (DFRobot Beetle)
- **I/O Expander:** MCP23018 (I2C) - buttons and LEDs
- **Sensors:** Temperature (DS18B20), Current, Battery voltage
- **Output:** ESC via PWM

---

## Architecture Diagram

```
                              ┌─────────────────┐
                              │   main.cpp      │
                              │ mainUnit.begin()│
                              └────────┬────────┘
                                       │
                              ┌────────▼────────┐
                              │  MoaMainUnit    │
                              │ (owns all hw)   │
                              └────────┬────────┘
                                       │
        ┌──────────────────────────────┼──────────────────────────────┐
        │                              │                              │
        ▼                              ▼                              ▼
┌───────────────┐            ┌─────────────────┐            ┌─────────────────┐
│  SensorTask   │            │    IOTask       │            │  ControlTask    │
│   (50ms)      │            │    (20ms)       │            │  (event-driven) │
│               │            │                 │            │                 │
│ TempControl   │            │ ButtonControl   │            │ StateMachine    │
│ BattControl   │            │ LedControl      │            │ Manager         │
│ CurrentControl│            │                 │            │                 │
└───────┬───────┘            └────────┬────────┘            └────────┬────────┘
        │                             │                              │
        └─────────────────────────────┼──────────────────────────────┘
                                      ▼
                    ┌─────────────────────────────────┐
                    │          Event Queue            │
                    │  (ControlCommand: type+cmd+val) │
                    └─────────────────┬───────────────┘
                                      │
                                      ▼
                    ┌─────────────────────────────────┐
                    │   MoaStateMachineManager        │
                    │   - Routes events by type       │
                    │   - Updates MoaDevicesManager   │
                    │   - Logs to MoaFlashLog         │
                    └─────────────────┬───────────────┘
                                      │
                    ┌─────────────────▼───────────────┐
                    │      MoaDevicesManager          │
                    │   (Output facade: LEDs, ESC)    │
                    └─────────────────────────────────┘
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

### Events (ControlCommand Format)

All events use the unified `ControlCommand` struct:

```cpp
struct ControlCommand {
    int controlType;   // Producer identifier (100-104)
    int commandType;   // Event type or ID
    int value;         // Measured value or event data
};
```

| controlType | Producer | commandType | value |
|-------------|----------|-------------|-------|
| 100 | MoaTimer | Timer ID | 0 (reserved) |
| 101 | MoaTempControl | CROSSED_ABOVE/BELOW | Temperature × 10 (°C) |
| 102 | MoaBattControl | LEVEL_HIGH/MED/LOW | Voltage (mV) |
| 103 | MoaCurrentControl | OVERCURRENT/NORMAL/REVERSE | Current × 10 (A) |
| 104 | MoaButtonControl | BUTTON_STOP/25/50/75/100 | PRESS/LONG_PRESS |

---

## FreeRTOS Tasks

| Task | Priority | Period | Responsibility |
|------|----------|--------|----------------|
| **SensorTask** | High | 50ms | Call `update()` on MoaTempControl, MoaBattControl, MoaCurrentControl |
| **IOTask** | Medium | 20ms | Call `update()` on MoaButtonControl, MoaLedControl |
| **ControlTask** | Medium | Event-driven | Process event queue, run StateMachine, update ESCController, call MoaFlashLog.update() |
| **BLETask** | Medium | Event-driven | [Future] GATT server, BLE commands → events |
| **WebTask** | Low | N/A | [Future] Config webserver (only in ConfigState) |

### Task Integration Example

```cpp
// SensorTask (50ms period)
void SensorTask(void* param) {
    for (;;) {
        tempControl.update();     // Pushes events on threshold crossing
        battControl.update();     // Pushes events on level change
        currentControl.update();  // Pushes events on overcurrent
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// IOTask (20ms period)
void IOTask(void* param) {
    for (;;) {
        buttonControl.update();   // Pushes events on button press/long-press
        ledControl.update();      // Drives LED blink timing
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ControlTask (event-driven)
void ControlTask(void* param) {
    ControlCommand cmd;
    for (;;) {
        if (xQueueReceive(eventQueue, &cmd, portMAX_DELAY)) {
            stateMachine.handleEvent(cmd);
            flashLog.update();    // Periodic flush check
        }
    }
}
```

---

## Synchronization

- **Event Queue:** Single FreeRTOS queue for all `ControlCommand` events → ControlTask
- **MCP23018 Mutex:** `MoaMcpDevice` class provides mutex-protected I2C access for MoaButtonControl and MoaLedControl
- **I2C Mutex:** [If needed] Additional mutex if other I2C devices share the bus
- **Config Mutex:** [Future] Protect shared config struct for web access

---

## Implementation Phases

### Phase 1: Core (V1) - COMPLETE ✅

- [x] Define `ControlCommand` struct
- [x] Implement MoaTimer (FreeRTOS xTimer wrapper with queue events)
- [x] Implement MoaTempControl (DS18B20 with averaging, hysteresis, queue events)
- [x] Implement MoaBattControl (ADC with averaging, thresholds, queue events)
- [x] Implement MoaCurrentControl (Hall effect sensor, bidirectional, queue events)
- [x] Implement MoaMcpDevice (thread-safe MCP23018 wrapper with mutex)
- [x] Implement MoaButtonControl (debounce, long-press detection, queue events)
- [x] Implement MoaLedControl (individual control, blink patterns, config mode indication)
- [x] Implement MoaFlashLog (LittleFS circular buffer, 128 entries, JSON export)
- [x] Create MoaMainUnit (central coordinator, owns all hardware)
- [x] Create MoaDevicesManager (output facade: LEDs, ESC, logging)
- [x] Create MoaStateMachineManager (event router)
- [x] Create FreeRTOS tasks (SensorTask, IOTask, ControlTask)
- [x] Create event queue and task integration
- [x] Reorganize project structure to match RTPBuit pattern
- [x] Fix build system (include paths, LittleFS dependency)
- [ ] **NEXT:** Wire ESCController to StateMachine (ramp control in SurfingState)
- [ ] **NEXT:** Implement state transitions and error states

### Phase 2: Refinement

- [x] Button debounce (configurable, default 50ms)
- [x] Long-press detection (configurable, default 5s)
- [x] LED blink patterns and config mode indication
- [x] Flash-based event logging with JSON export
- [ ] Tunable thresholds via configuration
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
│   ├── ControlCommand.h      # Unified event structure
│   ├── StateMachine/
│   │   ├── MoaState.h
│   │   ├── MoaStateMachine.h
│   │   ├── InitState.h
│   │   ├── IdleState.h
│   │   ├── SurfingState.h
│   │   ├── OverHeatingState.h
│   │   ├── OverCurrentState.h
│   │   └── BatteryLowState.h
│   ├── Devices/
│   │   ├── MoaTempControl.h       # DS18B20 temperature monitoring
│   │   ├── MoaBattControl.h       # Battery voltage monitoring
│   │   ├── MoaCurrentControl.h    # Hall effect current monitoring
│   │   ├── MoaButtonControl.h     # Button input with debounce/long-press
│   │   ├── MoaLedControl.h        # LED output with blink patterns
│   │   ├── MoaFlashLog.h          # Flash-based event logging
│   │   ├── MoaMcpDevice.h         # Thread-safe MCP23018 wrapper
│   │   ├── ESCController.h        # PWM ESC control with ramping
│   │   └── Adafruit_MCP23X18.h    # MCP23018 driver (copied from lib)
│   ├── Helpers/
│   │   ├── MoaMainUnit.h          # Central coordinator
│   │   ├── MoaDevicesManager.h    # Output facade (LEDs, ESC, log)
│   │   ├── MoaStateMachineManager.h  # Event router
│   │   └── MoaTimer.h             # FreeRTOS xTimer wrapper
│   ├── Tasks/
│   │   └── Tasks.h                # FreeRTOS task declarations
│   ├── PinMapping.h              # GPIO and MCP23018 pin definitions
│   ├── Constants.h               # Hardware constants and default values
│   └── Config.h                  [TODO]
├── src/
│   ├── StateMachine/
│   │   ├── MoaStateMachine.cpp
│   │   ├── InitState.cpp
│   │   ├── IdleState.cpp
│   │   ├── SurfingState.cpp
│   │   ├── OverHeatingState.cpp
│   │   ├── OverCurrentState.cpp
│   │   └── BatteryLowState.cpp
│   ├── Devices/
│   │   ├── MoaTempControl.cpp
│   │   ├── MoaBattControl.cpp
│   │   ├── MoaCurrentControl.cpp
│   │   ├── MoaButtonControl.cpp
│   │   ├── MoaLedControl.cpp
│   │   ├── MoaFlashLog.cpp
│   │   ├── MoaMcpDevice.cpp
│   │   ├── ESCController.cpp
│   │   └── Adafruit_MCP23X18.cpp
│   ├── Helpers/
│   │   ├── MoaMainUnit.cpp
│   │   ├── MoaDevicesManager.cpp
│   │   ├── MoaStateMachineManager.cpp
│   │   └── MoaTimer.cpp
│   ├── Tasks/
│   │   ├── SensorTask.cpp         # Sensor producer updates
│   │   ├── IOTask.cpp             # Button/LED updates
│   │   └── ControlTask.cpp        # Event queue processing
│   └── main.cpp                   # Ultra-clean entry point
├── lib/
│   ├── MCP23018/                  # Adafruit MCP23X18 base library
│   └── TempControl/               # [Legacy] Original temperature control
└── platformio.ini
```

---

## Key Design Principles

1. **State machine is source-agnostic** — doesn't know where events come from
2. **Single task owns state machine** — no mutex needed for state transitions
3. **Event queue decouples producers/consumers** — easy to add new input sources
4. **ConfigState is mutually exclusive** — WiFi/web only runs in dedicated mode
5. **I2C protected by mutex** — MoaMcpDevice provides thread-safe access
6. **Unified event format** — All producers use `ControlCommand` with consistent semantics
7. **Producer classes are self-contained** — Each handles its own averaging, hysteresis, and thresholds
8. **Critical events trigger immediate logging** — Overcurrent, overheat, errors flush to flash immediately
9. **MoaMainUnit owns everything** — Single coordinator class keeps main.cpp ultra-clean
10. **RTPBuit-inspired pattern** — DevicesManager facade + StateMachineManager router

---

## Notes

- ESP32-C3 is single-core; FreeRTOS provides preemptive multitasking, not parallelism
- BLE 5.0 only (no Classic Bluetooth)
- WiFi + BLE coexistence possible but not needed if ConfigState is exclusive
- Keep webserver minimal to conserve RAM
- Flash logging uses LittleFS with 128-entry circular buffer (~1KB)
- Long-press STOP button (configurable, default 5s) enters config mode

---

## Producer Classes Summary

| Class | Sensor/Source | Key Features |
|-------|---------------|---------------|
| **MoaTimer** | FreeRTOS xTimer | One-shot/periodic, timer ID in commandType |
| **MoaTempControl** | DS18B20 | Averaging, hysteresis, above/below threshold events |
| **MoaBattControl** | ADC + divider | Averaging, 3-level thresholds (HIGH/MED/LOW) |
| **MoaCurrentControl** | ACS759-200B Hall | Bidirectional, averaging, overcurrent detection |
| **MoaButtonControl** | MCP23018 Port A | Debounce, long-press, 5 buttons |
| **MoaLedControl** | MCP23018 Port B | 5 LEDs, blink patterns, config mode indication |
| **MoaFlashLog** | LittleFS | 128 entries, 1-min flush, JSON export |

---

*Last updated: 2026-01-31*

---

## Current Status & Next Steps

### ✅ Completed
- **Architecture**: FreeRTOS event-driven system with unified ControlCommand events
- **Project Structure**: Reorganized to match RTPBuit pattern (include/Devices/, src/Devices/)
- **All Producer Classes**: Temperature, Battery, Current, Button, LED, Timer, Flash logging
- **Core Infrastructure**: MoaMainUnit, MoaDevicesManager, MoaStateMachineManager
- **FreeRTOS Tasks**: SensorTask, IOTask, ControlTask with proper priorities
- **Build System**: PlatformIO configuration with correct include paths and dependencies
- **Compilation**: Project builds successfully with all components integrated

### 🎯 Next Steps for V1 Working Version
1. **Wire ESCController to StateMachine**: Implement throttle control in SurfingState
2. **Implement State Transitions**: Connect all states with proper event handling
3. **Add Error State Logic**: OverCurrent, OverHeating, BatteryLow behaviors
4. **Test Basic Operation**: Verify button input → state change → ESC output
5. **Add Serial Debug**: Optional telemetry for debugging

### 📋 Implementation Priority
1. **SurfingState ESC Control** (button → throttle level)
2. **State Machine Wiring** (event routing between states)
3. **Safety Features** (overcurrent/overheat shutdown)
4. **User Testing** (basic button control validation)
5. **Debug Output** (serial monitoring for development)

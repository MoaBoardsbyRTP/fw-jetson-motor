# Moa ESC Controller - Architecture Plan

## Overview

FreeRTOS-based architecture using the State Pattern for managing ESC control, sensor monitoring, and user input. Designed for extensibility to support future BLE control and WiFi configuration.

**Implementation Status:** Phase 1 & 2 complete - Core infrastructure fully implemented, all hardware abstraction classes complete. State machine fully functional with all 6 states handling events, cross-safety transitions, and LED indicators. Multi-stage button press (short, long 1s, very long 10s) implemented. LED state initialization bug fixed in error states (OverHeatingState, OverCurrentState, BatteryLowState now properly update LEDs on entry).

---

## Target Hardware

- **MCU:** ESP32-C3 (DFRobot Beetle)
- **I/O Expander:** MCP23018 (I2C) - buttons (Port A, interrupt-driven) and LEDs (Port B)
- **Sensors:** Temperature (DS18B20), Current (ACS759-200B), Battery voltage (ADC)
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
│  (non-block)  │            │  (interrupt)    │            │ Manager         │
│ BattControl   │            │ LedControl      │            │                 │
│ CurrentControl│            │ ESC Ramp Tick   │            │                 │
└───────┬───────┘            └────────┬────────┘            └────────┬────────┘
                                      │
                             MCP23018 INTA ──► ESP32 GPIO2 (ISR)
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
                    │   (Output facade: LEDs, ESC)      │
                    └─────────────────────────────────┘
                                      │
                    ┌─────────────────▼───────────────┐
                    │      Stats Queue (telemetry)    │
                    └─────────────────┬───────────────┘
                                      │
                                      ▼
                    ┌─────────────────────────────────┐
                    │      StatsTask (event-driven)   │
                    └─────────────────┬───────────────┘
                                      │
                                      ▼
                    ┌─────────────────────────────────┐
                    │      MoaStatsAggregator         │
                    │   (Thread-safe stats storage)   │
                    └─────────────────────────────────┘
```

---

## State Machine

### States

| State | Description | Status |
|-------|-------------|--------|
| **InitState** | Board locked. Long press STOP → Idle (unlock). Very long press STOP → Config mode | ✅ Complete |
| **IdleState** | Board unlocked, motor disengaged. Throttle buttons → Surfing. Long press STOP → Init (lock) | ✅ Complete |
| **SurfingState** | Motor active with ramped throttle. STOP → Idle. Safety events → error states. Timers for throttle timeout | ✅ Complete |
| **OverHeatingState** | Motor stopped. Temp below → Idle. Also handles overcurrent → OverCurrent, batt low → BatteryLow. Long press STOP → Init | ✅ Complete |
| **OverCurrentState** | Motor stopped. Current normal → Idle. Also handles temp above → OverHeating, batt low → BatteryLow. Long press STOP → Init | ✅ Complete |
| **BatteryLowState** | Motor stopped. Batt medium/high → Idle. Also handles overcurrent → OverCurrent, temp above → OverHeating. Long press STOP → Init | ✅ Complete |
| **ConfigState** | [Future] WiFi AP + webserver for configuration | ⏳ Not implemented |

### Events (ControlCommand Format)

All events use the unified `ControlCommand` struct:

```cpp
struct ControlCommand {
    int controlType;   // Producer identifier (100-104)
    int commandType;   // Event type or ID
    int value;         // Measured value or event data
};
```

All `CONTROL_TYPE_*`, `COMMAND_*`, and `BUTTON_EVENT_*` constants are defined in a single file: `ControlCommand.h`.

| controlType | Producer | commandType | value |
|-------------|----------|-------------|-------|
| 100 | MoaTimer | Timer ID | 0 (reserved) |
| 101 | MoaTempControl | COMMAND_TEMP_CROSSED_ABOVE/BELOW | Temperature × 10 (°C) |
| 102 | MoaBattControl | COMMAND_BATT_LEVEL_HIGH/MEDIUM/LOW | Voltage (mV) |
| 103 | MoaCurrentControl | COMMAND_CURRENT_OVERCURRENT/NORMAL/REVERSE | Current × 10 (A) |
| 104 | MoaButtonControl | COMMAND_BUTTON_STOP/25/50/75/100 | BUTTON_EVENT_PRESS/LONG_PRESS/VERY_LONG_PRESS/RELEASE |

---

## FreeRTOS Tasks

| Task | Priority | Period | Responsibility |
|------|----------|--------|----------------|
| **SensorTask** | High | 50ms | Call `update()` on MoaTempControl (non-blocking), MoaBattControl, MoaCurrentControl |
| **IOTask** | Medium | 20ms | Process button interrupts, check long-press, tick ESC ramp, update MoaLedControl |
| **ControlTask** | Medium | Event-driven | Process event queue, run StateMachine, call MoaFlashLog.update() |
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

// IOTask (20ms period) - Interrupt-driven buttons + ESC ramp
void IOTask(void* param) {
    for (;;) {
        if (buttonControl.isInterruptPending()) {
            buttonControl.processInterrupt();  // Read INTCAP+GPIO, debounce, push events
        }
        buttonControl.checkLongPress();        // Polled long-press detection
        devicesManager.updateESC();            // Tick ESC ramp stepper
        ledControl.update();                   // Drives LED blink timing
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
- **MCP23018 INTA Interrupt:** Hardware interrupt on ESP32 GPIO2 sets volatile flag; IOTask processes via I2C. INTA pin also polled for stuck-LOW recovery (missed FALLING edges)
- **MCP23018 Hardware Reset:** Dedicated reset line (GPIO10) for initialization and I2C error recovery
- **I2C Mutex:** [If needed] Additional mutex if other I2C devices share the bus
- **Config Mutex:** [Future] Protect shared config struct for web access

---

## Implementation Phases

### Phase 1: Core (V1) - COMPLETE ✅

#### Hardware Abstraction Layer - COMPLETE ✅
- [x] `ControlCommand` struct - Unified event structure
- [x] `MoaTimer` - FreeRTOS xTimer wrapper with queue events
- [x] `MoaTempControl` - DS18B20 with non-blocking async conversion, averaging, hysteresis, queue events, stats
- [x] `MoaBattControl` - ADC with averaging, 3-level thresholds, queue events, stats
- [x] `MoaCurrentControl` - Hall effect sensor, bidirectional, queue events, stats
- [x] `MoaMcpDevice` - Thread-safe MCP23018 wrapper with mutex, hardware reset, I2C error recovery
- [x] `MoaButtonControl` - Interrupt-driven via INTCAP+GPIO read (full interrupt clearing), per-button debounce, long-press detection, INTA pin polling for stuck-LOW recovery, queue events
- [x] `MoaLedControl` - Individual control, blink patterns, config mode indication
- [x] `MoaFlashLog` - LittleFS circular buffer, 128 entries, JSON export, critical flush
- [x] `MoaStatsAggregator` - Thread-safe stats storage with mutex
- [x] `StatsReading` - Telemetry structure for stats queue

#### Core Infrastructure - COMPLETE ✅
- [x] `MoaMainUnit` - Central coordinator, owns all hardware, creates queues/tasks
- [x] `MoaDevicesManager` - Output facade (LEDs, ESC, logging)
- [x] `MoaStateMachineManager` - Event router with full event handling
- [x] FreeRTOS tasks (SensorTask, IOTask, ControlTask, StatsTask)
- [x] Event queue and stats queue creation
- [x] Project structure reorganized to match RTPBuit pattern
- [x] Build system (PlatformIO) with correct include paths and dependencies

#### State Machine Framework - COMPLETE ✅
- [x] `MoaStateMachine` - State machine with all 6 state instances
- [x] `MoaState` - Abstract base class
- [x] All state classes: `InitState`, `IdleState`, `SurfingState`, `OverHeatingState`, `OverCurrentState`, `BatteryLowState`

#### ESC Integration - COMPLETE ✅
- [x] `ESCController` - PWM output with ramped throttle transitions, `getCurrentThrottle()` accessor
- [x] `MoaDevicesManager::setThrottleLevel()` - Converts percentage to duty cycle and initiates ramp
- [x] `MoaDevicesManager::updateESC()` - Ticks ramp stepper, called from IOTask every 20ms
- [x] `MoaDevicesManager::stopMotor()` - Immediate stop (cancels ramp)
- [x] Ramp rate configurable via `ESC_RAMP_RATE` (default 100%/s)

#### Pin Mapping & Constants - COMPLETE ✅
- [x] `PinMapping.h` - All GPIO and MCP23018 pin definitions
- [x] `Constants.h` - Hardware constants and default configuration values

### Phase 2: State Machine Logic - COMPLETE ✅

#### State Behavior Implementation - COMPLETE ✅
- [x] **InitState**: Board locked. Long press STOP (1s) unlocks → Idle. Very long press STOP (10s) → Config mode
- [x] **IdleState**: Board unlocked. Throttle buttons → Surfing. Long press STOP → Init (lock)
- [x] **SurfingState**: Throttle control with ramping, STOP → Idle, safety events → error states, timer-based throttle timeout
- [x] **OverHeatingState**: Stops motor. Temp below → Idle. Cross-handles overcurrent and battery low. Long press STOP → Init
- [x] **OverCurrentState**: Stops motor. Current normal → Idle. Cross-handles overheat and battery low. Long press STOP → Init
- [x] **BatteryLowState**: Stops motor. Batt medium/high → Idle. Cross-handles overcurrent and overheat. Long press STOP → Init

#### Button Multi-Stage Press - COMPLETE ✅
- [x] Short press (immediate): Throttle control in Idle/Surfing, STOP to disengage
- [x] Long press (1s, deferred): Board lock/unlock toggle (Init ↔ Idle), safety escape from error states
- [x] Very long press (10s): Config mode entry from Init state
- [x] Deferred long press firing: Long press event only fires on release if very long press threshold not reached

#### LED Signaling - COMPLETE ✅
- [x] Board locked (Init): Overcurrent LED solid ON
- [x] Board unlocked (Idle/Surfing): Overcurrent LED OFF
- [x] Overcurrent warning: Overcurrent LED blinks fast (250ms)
- [x] Overheat warning: Temperature LED blinks fast (250ms)
- [x] LED state cached in MoaDevicesManager and restored after wave animations via `refreshLedIndicators()`
- [x] Wave animation with fast mode for state transitions

#### ESC Integration - COMPLETE ✅
- [x] Wire ESCController to state machine via MoaDevicesManager
- [x] Ramped throttle transitions (percentage → duty cycle, configurable rate)
- [x] Emergency stop in stopMotor() (cancels ramp, immediate zero)
- [x] Ramp ticked from IOTask every 20ms

### Phase 3: Refinement - PARTIALLY COMPLETE 🔧

- [x] Button debounce (configurable, default 50ms, interrupt-driven with debounce window)
- [x] Long-press detection (1s default, deferred when very long press enabled)
- [x] Very long press detection (10s default)
- [x] LED blink patterns and config mode indication
- [x] LED board locked/unlocked signaling
- [x] Flash-based event logging with JSON export
- [x] Stats aggregator for telemetry
- [x] Command constants consolidated in ControlCommand.h (single source of truth)
- [ ] Tunable thresholds via configuration
- [ ] Serial debug output / telemetry streaming

### Phase 4: BLE Control (Future) - NOT STARTED ⏳

- [ ] Add BLETask with GATT server
- [ ] Define BLE characteristics for button simulation
- [ ] Map BLE commands to button events
- [ ] Optional: throttle control characteristic

### Phase 5: WiFi Configuration (Future) - NOT STARTED ⏳

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
│   ├── Helpers/
│   │   ├── Constants.h           # Hardware constants and defaults ✅
│   │   ├── ControlCommand.h      # Unified event structure + all CONTROL_TYPE/COMMAND constants ✅
│   │   ├── MoaDevicesManager.h   # Output facade (LEDs, ESC, log) ✅
│   │   ├── MoaMainUnit.h         # Central coordinator ✅
│   │   ├── MoaStateMachineManager.h # Event router ✅
│   │   ├── MoaStatsAggregator.h  # Thread-safe stats storage ✅
│   │   ├── MoaTimer.h            # FreeRTOS xTimer wrapper ✅
│   │   ├── PinMapping.h          # GPIO and MCP23018 pins ✅
│   │   └── StatsReading.h        # Telemetry structure ✅
│   ├── Devices/
│   │   ├── Adafruit_MCP23X18.h   # MCP23018 driver ✅
│   │   ├── ESCController.h       # PWM ESC control with ramping ✅
│   │   ├── MoaBattControl.h      # Battery voltage monitoring ✅
│   │   ├── MoaButtonControl.h    # Button input with debounce/long-press ✅
│   │   ├── MoaCurrentControl.h   # Hall effect current monitoring ✅
│   │   ├── MoaFlashLog.h         # Flash-based event logging ✅
│   │   ├── MoaLedControl.h       # LED output with blink patterns ✅
│   │   ├── MoaMcpDevice.h        # Thread-safe MCP23018 wrapper ✅
│   │   └── MoaTempControl.h      # DS18B20 temperature monitoring ✅
│   ├── StateMachine/
│   │   ├── BatteryLowState.h     # Battery low state ✅
│   │   ├── IdleState.h           # Idle state ✅
│   │   ├── InitState.h           # Initialization state ✅
│   │   ├── MoaState.h            # Abstract base class ✅
│   │   ├── MoaStateMachine.h     # State machine ✅
│   │   ├── MoaStateMachineManager.h # Event router ✅
│   │   ├── OverCurrentState.h    # Overcurrent state ✅
│   │   ├── OverHeatingState.h    # Overheating state ✅
│   │   └── SurfingState.h        # Normal operation state ✅
│   └── Tasks/
│       └── Tasks.h               # FreeRTOS task declarations ✅
├── src/
│   ├── Helpers/
│   │   ├── MoaDevicesManager.cpp ✅
│   │   ├── MoaMainUnit.cpp       ✅
│   │   ├── MoaStateMachineManager.cpp ✅
│   │   ├── MoaStatsAggregator.cpp ✅
│   │   └── MoaTimer.cpp          ✅
│   ├── Devices/
│   │   ├── Adafruit_MCP23X18.cpp ✅
│   │   ├── ESCController.cpp     ✅
│   │   ├── MoaBattControl.cpp    ✅
│   │   ├── MoaButtonControl.cpp  ✅
│   │   ├── MoaCurrentControl.cpp ✅
│   │   ├── MoaFlashLog.cpp       ✅
│   │   ├── MoaLedControl.cpp     ✅
│   │   ├── MoaMcpDevice.cpp      ✅
│   │   └── MoaTempControl.cpp    ✅
│   ├── StateMachine/
│   │   ├── BatteryLowState.cpp   ✅
│   │   ├── IdleState.cpp         ✅
│   │   ├── InitState.cpp         ✅
│   │   ├── MoaStateMachine.cpp   ✅
│   │   ├── OverCurrentState.cpp  ✅
│   │   ├── OverHeatingState.cpp  ✅
│   │   └── SurfingState.cpp      ✅
│   ├── Tasks/
│   │   ├── ControlTask.cpp       ✅
│   │   ├── IOTask.cpp            ✅
│   │   ├── SensorTask.cpp        ✅
│   │   └── StatsTask.cpp         ✅
│   └── main.cpp                  ✅
├── test/                         # Test directory
├── test_backup/                  # Backup test files
└── platformio.ini                # PlatformIO configuration ✅
```

**Legend:**
- ✅ Complete and fully implemented
- 🔧 Stub exists, needs implementation
- ⏳ Not implemented


---

## Key Design Principles

1. **State machine is source-agnostic** — doesn't know where events come from ✅
2. **Single task owns state machine** — no mutex needed for state transitions ✅
3. **Event queue decouples producers/consumers** — easy to add new input sources ✅
4. **Separate stats queue** — telemetry doesn't impact control events ✅
5. **I2C protected by mutex** — MoaMcpDevice provides thread-safe access ✅
5b. **Hardware reset for I2C recovery** — MCP23018 reset line (GPIO10) for initialization and error recovery ✅
5c. **Interrupt-driven button input** — MCP23018 INTA → ESP32 GPIO2 ISR, INTCAPA read clears interrupt ✅
6. **Stats protected by semaphore** — MoaStatsAggregator provides thread-safe access ✅
7. **Unified event format** — All producers use `ControlCommand` with consistent semantics ✅
8. **Producer classes are self-contained** — Each handles its own averaging, hysteresis, and thresholds ✅
9. **Critical events trigger immediate logging** — Overcurrent, overheat, errors flush to flash immediately ✅
10. **MoaMainUnit owns everything** — Single coordinator class keeps main.cpp ultra-clean ✅
11. **RTPBuit-inspired pattern** — DevicesManager facade + StateMachineManager router ✅

---

## Notes

- ESP32-C3 is single-core; FreeRTOS provides preemptive multitasking, not parallelism
- BLE 5.0 only (no Classic Bluetooth)
- WiFi + BLE coexistence possible but not needed if ConfigState is exclusive
- Keep webserver minimal to conserve RAM
- Flash logging uses LittleFS with 128-entry circular buffer (~1KB)
- Long-press STOP button (1s) toggles board lock/unlock (Init ↔ Idle)
- Very long press STOP button (10s) enters config mode from Init state
- Long press event is deferred when very long press is enabled (fires on release if threshold not reached)
- Button interrupts reduce I2C bus load: I2C reads only on button press/release, not every 20ms
- MCP23018 INTA is active-low, open-drain; ESP32 GPIO2 configured with INPUT_PULLUP
- INTCAP register read alone may not fully clear the MCP23018 interrupt; reading GPIO afterwards ensures full clearing
- `isInterruptPending()` also polls the INTA pin state to catch stuck-LOW conditions (missed FALLING edges)
- Hardware reset pin (GPIO10) pulses LOW for 2μs, then 1ms stabilization delay

---

## Producer Classes Summary

| Class | Sensor/Source | Key Features | Status |
|-------|---------------|--------------|--------|
| **MoaTimer** | FreeRTOS xTimer | One-shot/periodic, timer ID in commandType | ✅ Complete |
| **MoaTempControl** | DS18B20 | Non-blocking async conversion, averaging, hysteresis, above/below threshold events, stats | ✅ Complete |
| **MoaBattControl** | ADC + divider | Averaging, 3-level thresholds (HIGH/MED/LOW), stats | ✅ Complete |
| **MoaCurrentControl** | ACS759-200B Hall | Bidirectional, averaging, overcurrent detection, stats | ✅ Complete |
| **MoaButtonControl** | MCP23018 Port A | Interrupt-driven (INTA), INTCAP+GPIO read for full clearing, per-button debounce, INTA polling for stuck-LOW, long-press (1s), very long press (10s), deferred firing, 5 buttons | ✅ Complete |
| **MoaLedControl** | MCP23018 Port B | 5 LEDs, blink patterns, config mode indication | ✅ Complete |
| **MoaFlashLog** | LittleFS | 128 entries, 1-min flush, JSON export, critical flush | ✅ Complete |
| **MoaStatsAggregator** | Stats queue | Thread-safe storage, mutex-protected access | ✅ Complete |

---

## Current Status Summary

### ✅ Completed (Ready for Use)
- All hardware abstraction classes fully implemented
- All sensor producers with averaging, hysteresis, and event generation
- Non-blocking DS18B20 temperature reading (async two-phase state machine)
- Interrupt-driven button input via MCP23018 INTA with per-button debounce, long-press (1s), very long press (10s), deferred firing, and INTA pin polling for stuck-LOW recovery
- MCP23018 pullup configuration fixed (`INPUT_PULLUP` properly enables pullups)
- MCP23018 interrupt fully cleared by reading both INTCAP and GPIO registers
- MCP23018 hardware reset line for initialization and I2C error recovery
- Custom `Adafruit_MCP23X18` class with `readIntCapA()`/`readIntCapB()` for interrupt capture registers
- LED output with blink patterns, board locked/unlocked signaling, warning blinks for overcurrent/overheat
- LED state caching and restoration after wave animations
- Flash logging with circular buffer
- FreeRTOS task infrastructure
- Event routing and state machine framework
- **Full state machine logic**: All 6 states with complete event handling and cross-safety transitions
- Stats aggregation for telemetry
- ESC PWM with percentage-to-duty-cycle conversion and ramped throttle transitions
- ESC ramp ticked from IOTask (20ms), configurable rate via `ESC_RAMP_RATE`
- Command constants consolidated in `ControlCommand.h` (single source of truth)
- Build system configured

### ⏳ Not Started
- ConfigState for WiFi configuration
- BLE control interface
- Web server for configuration
- Serial telemetry streaming

---

*Last updated: 2026-02-18*

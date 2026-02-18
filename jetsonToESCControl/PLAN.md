# Moa ESC Controller - Development Plan

**Project:** jetsonToESCControl  
**Last Updated:** 2026-02-18  
**Based on:** Complete codebase analysis + recent implementation work

---

## Executive Summary

The jetsonToESCControl project has a **fully implemented Phase 1** with all hardware abstraction classes, FreeRTOS infrastructure, and the state machine framework complete. The system is event-driven using `ControlCommand` structs via FreeRTOS queues, with thread-safe I2C access via mutex-protected `MoaMcpDevice`. Button inputs are **interrupt-driven** via MCP23018 INTA pin, with hardware reset support for I2C error recovery.

**Current Status:** Phase 1 & 2 complete. All 6 states fully implemented with cross-safety transitions, multi-stage button press (1s long, 10s very long), LED board lock/unlock signaling, and command constants consolidated in `ControlCommand.h`. Ready for Phase 3 (configuration & telemetry).

---

## Current Status Overview

| Component | Status | Notes |
|-----------|--------|-------|
| Hardware Abstraction (8 classes) | ✅ Complete | All sensors, I/O (interrupt-driven buttons), logging, stats fully functional |
| FreeRTOS Tasks (4 tasks) | ✅ Complete | Sensor, IO, Control, Stats tasks running |
| Event System | ✅ Complete | ControlCommand + ControlEventType unified events |
| State Machine Framework | ✅ Complete | All 6 state classes instantiated |
| **Core State Logic** | ✅ **Complete** | InitState, IdleState, SurfingState handle buttons + ESC |
| **Error State Logic** | ✅ **Complete** | OverHeating, OverCurrent, BatteryLow with cross-safety transitions |
| **Button Multi-Stage** | ✅ **Complete** | Short press, long press (1s), very long press (10s), deferred firing |
| **LED Signaling** | ✅ **Complete** | Board locked/unlocked, overcurrent/overheat warning blinks |
| **Constants Refactor** | ✅ **Complete** | All CONTROL_TYPE/COMMAND defines in ControlCommand.h |
| ESC Integration | ✅ Complete | Ramped throttle via MoaDevicesManager, ticked from IOTask |
| DS18B20 Non-blocking | ✅ Complete | Async two-phase state machine, no longer blocks SensorTask |
| Button Interrupt Fixes | ✅ Complete | Pullup fix, INTCAP+GPIO clearing, INTA polling |
| Config/ BLE/ WiFi | ⏳ Not Started | Future phases |

---

## Phase Breakdown

### Phase 1: Core Infrastructure ✅ COMPLETE

**All components fully implemented and tested:**

#### Sensor Producers (All with stats support)
- ✅ `MoaTempControl` - DS18B20, **non-blocking async conversion**, averaging, hysteresis, above/below events
- ✅ `MoaBattControl` - ADC, 3-level thresholds (HIGH/MED/LOW), hysteresis
- ✅ `MoaCurrentControl` - ACS759-200B Hall sensor, bidirectional, overcurrent detection

#### I/O Controllers
- ✅ `MoaButtonControl` - 5 buttons via MCP23018, interrupt-driven (INTA), **INTCAP+GPIO read for full clearing**, per-button debounce, **INTA pin polling for stuck-LOW recovery**, long-press (1s), very long press (10s), deferred firing
- ✅ `MoaLedControl` - 5 LEDs via MCP23018, blink patterns, config mode indication
- ✅ `MoaMcpDevice` - Thread-safe I2C wrapper with FreeRTOS mutex, hardware reset (GPIO10), I2C error recovery
- ✅ `Adafruit_MCP23X18` - Custom MCP23018 driver with `readIntCapA()`/`readIntCapB()` for interrupt capture registers

#### System Services
- ✅ `MoaFlashLog` - LittleFS circular buffer, 128 entries, JSON export, critical flush
- ✅ `MoaTimer` - FreeRTOS xTimer wrapper with queue events
- ✅ `MoaStatsAggregator` - Thread-safe stats storage with semaphore
- ✅ `StatsReading` - Telemetry data structure

#### Core Classes
- ✅ `MoaMainUnit` - Central coordinator, creates queues/tasks, owns all hardware
- ✅ `MoaDevicesManager` - Output facade for LEDs, ESC (with ramped throttle), logging
- ✅ `MoaStateMachineManager` - Event router with full event handling
- ✅ `MoaStateMachine` - State machine with all 6 state instances
- ✅ `MoaState` - Abstract base class for all states

#### Infrastructure
- ✅ `ControlCommand` - Unified event structure + all CONTROL_TYPE/COMMAND constants (single source of truth)
- ✅ `PinMapping.h` - Complete GPIO and MCP23018 definitions
- ✅ `Constants.h` - All hardware constants and defaults
- ✅ FreeRTOS tasks: `SensorTask` (50ms, non-blocking temp), `IOTask` (20ms, interrupt buttons + ESC ramp tick), `ControlTask`, `StatsTask`
- ✅ PlatformIO build system with all dependencies

---

### Phase 2: State Machine Logic ✅ COMPLETE

**All states fully implemented with cross-safety transitions**

#### Core States - COMPLETE ✅

**2.1 InitState** ✅
- [x] Board locked on enter (overcurrent LED solid ON, wave animation)
- [x] Long press STOP (1s): Unlock board → Idle (fast wave animation)
- [x] Very long press STOP (10s): Enter config mode
- [x] Ignores all other events (overcurrent, temp, battery, timer)

**2.2 IdleState** ✅
- [x] Board unlocked on enter, motor disengaged
- [x] Throttle buttons (25/50/75/100%): Engage throttle → Surfing
- [x] Long press STOP: Lock board → Init
- [x] Short press STOP: Ignored (already idle)

**2.3 SurfingState** ✅
- [x] Throttle buttons: Update throttle via ramped transition
- [x] STOP: Disengage throttle → Idle
- [x] Overcurrent (COMMAND_CURRENT_OVERCURRENT): Disengage → OverCurrent
- [x] Temperature above (COMMAND_TEMP_CROSSED_ABOVE): Disengage → OverHeating
- [x] Battery low (COMMAND_BATT_LEVEL_LOW): → BatteryLow
- [x] Timer expired: Throttle timeout → Idle, full throttle step-down

#### Error States - COMPLETE ✅

**2.4 OverHeatingState** ✅
- [x] `onEnter()`: Stop motor
- [x] Temperature below (COMMAND_TEMP_CROSSED_BELOW): Stop motor → Idle
- [x] Overcurrent: Stop motor → OverCurrent
- [x] Battery low: Stop motor → BatteryLow
- [x] Long press STOP: Stop motor → Init (lock board)

**2.5 OverCurrentState** ✅
- [x] `onEnter()`: Stop motor
- [x] Current normal (COMMAND_CURRENT_NORMAL): Disengage throttle → Idle
- [x] Temperature above: Stop motor → OverHeating
- [x] Battery low: Stop motor → BatteryLow
- [x] Long press STOP: Stop motor → Init (lock board)

**2.6 BatteryLowState** ✅
- [x] `onEnter()`: Stop motor
- [x] Battery medium or high: Stop motor → Idle
- [x] Overcurrent: Stop motor → OverCurrent
- [x] Temperature above: Stop motor → OverHeating
- [x] Long press STOP: Stop motor → Init (lock board)

#### Button Multi-Stage Press - COMPLETE ✅
- [x] Short press: Immediate throttle control and STOP
- [x] Long press (1s, deferred): Board lock/unlock, safety escape from error states
- [x] Very long press (10s): Config mode from Init
- [x] Deferred long press: Only fires on release if very long press threshold not reached

#### LED Signaling - COMPLETE ✅
- [x] Board locked (Init): Overcurrent LED solid ON
- [x] Board unlocked (Idle/Surfing): Overcurrent LED OFF
- [x] Overcurrent warning: Overcurrent LED blinks fast (250ms)
- [x] Overheat warning: Temperature LED blinks fast (250ms)
- [x] LED state cached and restored after wave animations
- [x] Wave animation with fast mode for unlock transition

#### ESC Integration - COMPLETE ✅
- [x] `MoaDevicesManager::setThrottleLevel()` converts percentage → duty cycle, initiates ramp via `setRampThrottle()`
- [x] `MoaDevicesManager::updateESC()` ticks ramp stepper, called from IOTask every 20ms
- [x] `MoaDevicesManager::stopMotor()` cancels ramp and sets throttle to zero immediately
- [x] `ESCController::getCurrentThrottle()` accessor for ramp delta calculation
- [x] Ramp rate configurable via `ESC_RAMP_RATE` (currently 100%/s)

---

### Phase 3: Configuration & Telemetry ⏳ PENDING

**Priority: MEDIUM - Enhancement after basic functionality works**

- [ ] Tunable thresholds via NVS configuration
- [ ] Serial telemetry streaming (JSON format)
- [ ] Runtime threshold adjustment
- [ ] Stats history/graphing

---

### Phase 4: BLE Control ⏳ FUTURE

**Priority: LOW - Nice to have after V1 release**

- [ ] Add `BLETask` with GATT server
- [ ] Define BLE characteristics for button simulation
- [ ] Map BLE commands to `ControlCommand` events
- [ ] Optional: Throttle control characteristic (security concern)

---

### Phase 5: WiFi Configuration ⏳ FUTURE

**Priority: LOW - Alternative to serial config**

- [ ] Add `ConfigState` to state machine
- [ ] Entry via long-press STOP (5s) or boot combo
- [ ] Start WiFi AP, run AsyncWebServer
- [ ] Serve config page (thresholds, parameters)
- [ ] Store config in NVS
- [ ] Reboot to normal operation on save

---

## Implementation Roadmap

### Completed

1. ~~**Implement SurfingState**~~ ✅ Done
2. ~~**Implement IdleState**~~ ✅ Done
3. ~~**Implement InitState**~~ ✅ Done
4. ~~**ESC Ramping Integration**~~ ✅ Done
5. ~~**Implement Error States**~~ ✅ Done (OverCurrent, OverHeating, BatteryLow with cross-safety)
6. ~~**LED State Indicators**~~ ✅ Done (locked/unlocked, warning blinks, cached state)
7. ~~**Button Multi-Stage Press**~~ ✅ Done (1s long, 10s very long, deferred firing)
8. ~~**Command Constants Refactor**~~ ✅ Done (consolidated in ControlCommand.h)

### Immediate Next Steps

9. **Safety Event Testing** (2-3 hours)
   - Button → State → ESC verification
   - Safety cutoff testing (overcurrent, overheat, low battery)
   - LED indication verification
   - Cross-safety transition testing (e.g. overheat while in overcurrent)

10. **Serial Debug Output** (1 hour)
    - State transition logging
    - Sensor value streaming

### Short Term (Next 2 Weeks)

11. **Configuration System** (3-4 hours)
    - NVS storage for thresholds
    - Serial command interface

12. **Telemetry System** (2-3 hours)
    - Stats history
    - JSON export

---

## Technical Details

### Event System Reference

```cpp
// ControlCommand struct (unified event)
struct ControlCommand {
    int controlType;   // 100=Timer, 101=Temp, 102=Batt, 103=Current, 104=Button
    int commandType;   // Event type (see ControlEventType)
    int value;         // Scaled value or event data
};

// All constants defined in ControlCommand.h:
// CONTROL_TYPE_TIMER=100, CONTROL_TYPE_TEMPERATURE=101, CONTROL_TYPE_BATTERY=102,
// CONTROL_TYPE_CURRENT=103, CONTROL_TYPE_BUTTON=104
//
// COMMAND_TEMP_CROSSED_ABOVE=1, COMMAND_TEMP_CROSSED_BELOW=2
// COMMAND_BATT_LEVEL_HIGH=1, COMMAND_BATT_LEVEL_MEDIUM=2, COMMAND_BATT_LEVEL_LOW=3
// COMMAND_CURRENT_OVERCURRENT=1, COMMAND_CURRENT_NORMAL=2, COMMAND_CURRENT_REVERSE_OVERCURRENT=3
// COMMAND_BUTTON_STOP=1, COMMAND_BUTTON_25=2, ..., COMMAND_BUTTON_100=5
// BUTTON_EVENT_PRESS=1, BUTTON_EVENT_LONG_PRESS=2, BUTTON_EVENT_RELEASE=3, BUTTON_EVENT_VERY_LONG_PRESS=4
```

### Task Priorities

| Task | Priority | Stack | Period |
|------|----------|-------|--------|
| SensorTask | 3 (High) | 4096 | 50ms |
| IOTask | 2 (Medium) | 4096 | 20ms |
| ControlTask | 2 (Medium) | 4096 | Event-driven |
| StatsTask | 1 (Low) | 2048 | Event-driven |

### Key Thresholds (from Constants.h)

| Parameter | Default | Notes |
|-----------|---------|-------|
| Temp Warning | 60°C | Transition to OverHeatingState |
| Temp Recovery | 50°C | Return from OverHeatingState |
| Current Limit | 180A | Transition to OverCurrentState |
| Battery Low | 11.1V | Transition to BatteryLowState |
| Button Debounce | 50ms | Configurable |
| Long Press | 1s | Board lock/unlock toggle |
| Very Long Press | 10s | Config mode entry (from Init) |

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| State transition bugs | Medium | High | Thorough testing, clear state diagram |
| ESC ramping timing | Low | Medium | Implemented and tested, rate configurable via ESC_RAMP_RATE |
| Sensor threshold tuning | High | Medium | Make thresholds configurable |
| I2C mutex contention | Low | Low | Already implemented, monitor if issues arise |
| I2C bus lockup | Low | High | Hardware reset line (GPIO10) + `MoaMcpDevice::recover()` for automatic recovery |

---

## Success Criteria

### Phase 2 Complete When: ✅ ALL MET
- [x] Button press initiates ESC output in SurfingState
- [x] Different buttons set different throttle levels (25/50/75/100%)
- [x] STOP button stops ESC and returns to IdleState
- [x] Throttle transitions are ramped (configurable rate, currently 100%/s)
- [x] Overcurrent immediately stops ESC
- [x] Overheating stops ESC and allows recovery
- [x] Low battery stops ESC
- [x] LED indicators reflect current state (locked/unlocked, warning blinks)
- [x] All state transitions logged to flash
- [x] Cross-safety transitions between error states
- [x] Long press STOP escapes any error state to Init

### V1 Release When:
- [x] Basic button control works reliably (multi-stage press)
- [ ] Safety cutoffs tested and functional
- [ ] Serial debug output available
- [x] Documentation updated

---

## Appendix: State Transition Diagram

```
                    ┌─────────────┐
                    │  InitState  │ (Board Locked)
                    │  LED: ON    │
                    └──────┬──────┘
                           │ long press STOP (1s)
                           ▼
                    ┌─────────────┐
         ┌─────────│  IdleState  │◄──────────────────────────────┐
         │         │ (Unlocked)  │                               │
         │         └──────┬──────┘                               │
         │                │ throttle button                      │
         │                ▼                                       │
         │         ┌─────────────┐     safety event              │
         │         │ SurfingState│ ─────────► Error States ───────┤
         │         └──────┬──────┘                               │
         │                │ STOP press                           │
         └────────────────┘                                       │
                                                                 │
    Error States (all stop motor, all: long press STOP → Init)   │
    ┌─────────────────────────────────────────────────────────────┘
    │
    ▼
┌──────────────┐    temp below    ┌──────────────┐
│ OverHeating  │ ────────────────►│   IdleState  │
│    State     │                  │              │
└─────┬────────┘                  └──────────────┘
      │ overcurrent → OverCurrentState
      │ batt low → BatteryLowState

┌──────────────┐  current normal  ┌──────────────┐
│ OverCurrent  │ ────────────────►│   IdleState  │
│    State     │                  │              │
└─────┬────────┘                  └──────────────┘
      │ temp above → OverHeatingState
      │ batt low → BatteryLowState

┌──────────────┐  batt med/high   ┌──────────────┐
│  BatteryLow  │ ────────────────►│   IdleState  │
│    State     │                  │              │
└─────┬────────┘                  └──────────────┘
      │ overcurrent → OverCurrentState
      │ temp above → OverHeatingState
```

---

*Generated: 2026-02-05*  
*Updated: 2026-02-06 - Interrupt-driven buttons, hardware reset, custom Adafruit_MCP23X18*  
*Updated: 2026-02-11 - Non-blocking DS18B20, button interrupt fixes (pullup + INTCAP clearing + INTA polling), PWM percentage fix, ESC ramping integration, core state machine functional*  
*Updated: 2026-02-12 - Full state machine implementation (all 6 states with cross-safety transitions), multi-stage button press (1s/10s), LED board lock/unlock signaling, command constants consolidated in ControlCommand.h*  
*Updated: 2026-02-18 - Fixed LED state initialization bug in error states (OverHeatingState, OverCurrentState, BatteryLowState) - added missing `indicateOverheat()`, `indicateOvercurrent()`, `showBatteryLevel()`, and `refreshLedIndicators()` calls in `onEnter()` methods*  
*Based on: Complete codebase analysis + implementation*

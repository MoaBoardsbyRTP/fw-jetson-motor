# Moa ESC Controller - Development Plan

**Project:** jetsonToESCControl  
**Last Updated:** 2026-02-11  
**Based on:** Complete codebase analysis + recent implementation work

---

## Executive Summary

The jetsonToESCControl project has a **fully implemented Phase 1** with all hardware abstraction classes, FreeRTOS infrastructure, and the state machine framework complete. The system is event-driven using `ControlCommand` structs via FreeRTOS queues, with thread-safe I2C access via mutex-protected `MoaMcpDevice`. Button inputs are **interrupt-driven** via MCP23018 INTA pin, with hardware reset support for I2C error recovery.

**Current Blocker:** Error state classes (OverHeatingState, OverCurrentState, BatteryLowState) still have empty event handlers. Core flow (Init → Idle → Surfing) is functional with ESC ramping.

---

## Current Status Overview

| Component | Status | Notes |
|-----------|--------|-------|
| Hardware Abstraction (8 classes) | ✅ Complete | All sensors, I/O (interrupt-driven buttons), logging, stats fully functional |
| FreeRTOS Tasks (4 tasks) | ✅ Complete | Sensor, IO, Control, Stats tasks running |
| Event System | ✅ Complete | ControlCommand + ControlEventType unified events |
| State Machine Framework | ✅ Complete | All 6 state classes instantiated |
| **Core State Logic** | ✅ **Functional** | InitState, IdleState, SurfingState handle buttons + ESC |
| **Error State Logic** | 🔧 **Stubs** | OverHeating, OverCurrent, BatteryLow handlers empty |
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
- ✅ `MoaButtonControl` - 5 buttons via MCP23018, interrupt-driven (INTA), **INTCAP+GPIO read for full clearing**, per-button debounce, **INTA pin polling for stuck-LOW recovery**, long-press (5s)
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
- ✅ `ControlCommand` + `ControlEventType` - Unified event system
- ✅ `PinMapping.h` - Complete GPIO and MCP23018 definitions
- ✅ `Constants.h` - All hardware constants and defaults
- ✅ FreeRTOS tasks: `SensorTask` (50ms, non-blocking temp), `IOTask` (20ms, interrupt buttons + ESC ramp tick), `ControlTask`, `StatsTask`
- ✅ PlatformIO build system with all dependencies

---

### Phase 2: State Machine Logic 🔧 IN PROGRESS

**Priority: HIGH - Core flow works, error states still need implementation**

#### Core States - COMPLETE ✅

**2.1 InitState** ✅
- [x] Transitions to `IdleState` on any non-STOP button press

**2.2 IdleState** ✅
- [x] Handle button presses:
  - STOP: Stay in Idle, stop motor
  - 25%/50%/75%/100%: Set throttle via ramped transition, transition to `SurfingState`
- [ ] Handle safety events (overheat, overcurrent, low battery): Transition to appropriate error state

**2.3 SurfingState** ✅
- [x] Handle button presses:
  - STOP: Stop motor, transition to `IdleState`
  - 25%/50%/75%/100%: Update throttle via ramped transition
- [ ] Handle `timerExpired`: For future auto-stop feature
- [ ] Handle safety events:
  - `overcurrentDetected`: Transition to `OverCurrentState`
  - `temperatureCrossedLimit` (above): Transition to `OverHeatingState`
  - `batteryLevelCrossedLimit` (LOW): Transition to `BatteryLowState`

#### Error States - NOT STARTED ⏳

**2.4 OverHeatingState** ⏳
- [ ] Implement `onEnter()` - Reduce or stop ESC, show overheat LED pattern
- [ ] Monitor temperature via `temperatureCrossedLimit` (below) to return to `IdleState`
- [ ] Handle STOP button: Transition to `IdleState`

**2.5 OverCurrentState** ⏳
- [ ] Implement `onEnter()` - Stop ESC immediately, show overcurrent LED pattern
- [ ] Require STOP button press to transition to `IdleState` (manual reset)
- [ ] Log critical event to flash immediately

**2.6 BatteryLowState** ⏳
- [ ] Implement `onEnter()` - Stop ESC, show low battery LED pattern
- [ ] Handle battery level changes (charging/power cycle): Transition to `IdleState`
- [ ] Log critical event to flash immediately

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

### Immediate Next Steps (This Week)

1. ~~**Implement SurfingState**~~ ✅ Done
2. ~~**Implement IdleState**~~ ✅ Done
3. ~~**Implement InitState**~~ ✅ Done
4. ~~**ESC Ramping Integration**~~ ✅ Done

5. **Implement Error States** (1-2 hours)
   - OverCurrentState: Stop ESC, require reset
   - OverHeatingState: Stop ESC, auto-recovery
   - BatteryLowState: Stop ESC, indicate error

6. **LED State Indicators** (1 hour)
   - Show different LED patterns per state
   - Blink patterns for error states

### Short Term (Next 2 Weeks)

7. **Safety Event Testing** (2-3 hours)
   - Button → State → ESC verification
   - Safety cutoff testing (overcurrent, overheat, low battery)
   - LED indication verification

8. **Serial Debug Output** (1 hour)
   - State transition logging
   - Sensor value streaming

### Medium Term (Next Month)

8. **Configuration System** (3-4 hours)
   - NVS storage for thresholds
   - Serial command interface

9. **Telemetry System** (2-3 hours)
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

// ControlEventType enum
enum ControlEventType {
    CROSSED_ABOVE = 1, CROSSED_BELOW = 2,
    LEVEL_HIGH = 10, LEVEL_MEDIUM = 11, LEVEL_LOW = 12,
    OVERCURRENT = 20, REVERSE_OVERCURRENT = 21, NORMAL_CURRENT = 22,
    BUTTON_STOP = 30, BUTTON_25 = 31, BUTTON_50 = 32, BUTTON_75 = 33, BUTTON_100 = 34,
    BUTTON_PRESS = 40, BUTTON_LONG_PRESS = 41,
    LOG_FLUSH = 50, CONFIG_MODE = 51
};
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
| Long Press | 5s | Config mode entry |

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

### Phase 2 Complete When:
- [x] Button press initiates ESC output in SurfingState
- [x] Different buttons set different throttle levels (25/50/75/100%)
- [x] STOP button stops ESC and returns to IdleState
- [x] Throttle transitions are ramped (configurable rate, currently 100%/s)
- [ ] Overcurrent immediately stops ESC
- [ ] Overheating stops ESC and allows recovery
- [ ] Low battery stops ESC
- [ ] LED indicators reflect current state
- [ ] All state transitions logged to flash

### V1 Release When:
- [ ] Basic button control works reliably
- [ ] Safety cutoffs tested and functional
- [ ] Serial debug output available
- [ ] Documentation updated

---

## Appendix: State Transition Diagram

```
                    ┌─────────────┐
                    │  InitState  │
                    └──────┬──────┘
                           │ init complete
                           ▼
                    ┌─────────────┐
         ┌─────────│  IdleState  │◄──────────────────────────────┐
         │         └──────┬──────┘                               │
         │                │ button 25/50/75/100%                 │
         │                ▼                                       │
         │         ┌─────────────┐     safety event              │
         │         │ SurfingState│ ─────────► Error States ───────┤
         │         └──────┬──────┘                               │
         │                │ button STOP                          │
         └────────────────┘                                       │
                                                                 │
    ┌─────────────────────────────────────────────────────────────┘
    │
    ▼
┌──────────────┐    temp below    ┌──────────────┐
│ OverHeating  │ ────────────────►│   IdleState  │
│    State     │◄─── temp above ───│              │
└──────────────┘                  └──────────────┘

┌──────────────┐    button STOP   ┌──────────────┐
│ OverCurrent  │ ────────────────►│   IdleState  │
│    State     │ (manual reset)   │              │
└──────────────┘                  └──────────────┘

┌──────────────┐    power cycle   ┌──────────────┐
│  BatteryLow  │ ────────────────►│   IdleState  │
│    State     │   or charge      │              │
└──────────────┘                  └──────────────┘
```

---

*Generated: 2026-02-05*  
*Updated: 2026-02-06 - Interrupt-driven buttons, hardware reset, custom Adafruit_MCP23X18*  
*Updated: 2026-02-11 - Non-blocking DS18B20, button interrupt fixes (pullup + INTCAP clearing + INTA polling), PWM percentage fix, ESC ramping integration, core state machine functional*  
*Based on: Complete codebase analysis + implementation*

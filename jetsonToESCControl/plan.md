# ConfigState + WiFi-Triggered OTA — Implementation Plan

## Goal

Create a new `ConfigState` in the state machine that:
1. **Activates WiFi** (with your 8.5dBm TX power workaround) only when in this state
2. **Runs a minimal webserver** for configuration (Phase 5 extension)
3. **Enables OTA** firmware updates while in this state
4. **Tears down WiFi** when exiting back to normal operation

**Entry:** From `InitState` via very long press STOP (replacing the existing `// TODO` at InitState.cpp:29)
**Exit:** Long press STOP → back to `InitState`

---

## Architecture

### State Diagram

```
                    ┌──────────────────┐
     ┌──────────────│    InitState     │──────────────┐
     │  STOP long   │  (board locked)  │  STOP v.long │
     │  press       └──────────────────┘   press      │
     │                       ▲                        │
     │                       │ STOP long press        ▼
     │              ┌────────┴─────────┐    ┌─────────────────┐
     │              │   IdleState      │    │   ConfigState   │
     │              │  (motor off)     │    │  WiFi AP + OTA  │
     │              └────────┬─────────┘    │  Webserver      │
     │                       │ throttle btn └─────────────────┘
     │              ┌────────▼─────────┐              ▲
     │              │  SurfingState    │              │
     │              │  (motor active)  │              │ safety events
     │              └──────────────────┘              │
     │                       │                        │
     │              ┌────────▼─────────┐              │
     └─────────────►│  Error States    │──────────────┘
                    └──────────────────┘
```

---

## Files to Create

| File | Purpose |
|------|---------|
| `include/StateMachine/ConfigState.h` | ConfigState class declaration |
| `src/StateMachine/ConfigState.cpp` | ConfigState implementation |

---

## Files to Modify

### 1. MoaOTAManager.h & .cpp

Add to class:
```cpp
public:
    void stop();           // Tear down WiFi AP
    bool isActive() const; // Check if AP is running
private:
    bool _active;          // Track if AP is up
```

Keep WIFI_POWER_8_5dBm — your antenna workaround.

In stop():
```cpp
void MoaOTAManager::stop() {
    if (_active) {
        ArduinoOTA.end();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        _active = false;
        _apActive = false;
        ESP_LOGI(TAG, "WiFi AP stopped");
    }
}
```

In begin(): Set _active = true after successful AP start.

### 2. MoaStateMachine.h & .cpp

Header:
```cpp
private:
    MoaState* _configState;
public:
    MoaState* getConfigState();
```

Constructor: Create ConfigState instance.
setState(): Add ConfigState to name logging.

### 3. InitState.cpp

Replace the TODO (line 25-30):
```cpp
} else if (command.commandType == COMMAND_BUTTON_STOP && command.value == BUTTON_EVENT_VERY_LONG_PRESS) {
    ESP_LOGI(TAG, "Entering Config State");
    _moaMachine.setState(_moaMachine.getConfigState());
}
```

### 4. MoaDevicesManager.h & .cpp

Add OTA manager reference:
```cpp
// Constructor: add MoaOTAManager& otaManager parameter
void startOTA();   // Calls _otaManager.begin()
void stopOTA();    // Calls _otaManager.stop()
void exitConfigMode();  // Stop LED config blink
```

### 5. MoaMainUnit.cpp

Update _devicesManager constructor to pass _otaManager reference.

### 6. OtaTask.cpp

Remove begin() from task start:
```cpp
void OtaTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);
    ESP_LOGI(TAG, "OtaTask started");
    // No begin() here — ConfigState controls lifecycle
    for (;;) {
        unit->getOTAManager().handle();
        vTaskDelay(pdMS_TO_TICKS(TASK_OTA_PERIOD_MS));
    }
}
```

---

## ConfigState Implementation

ConfigState.h:
```cpp
#pragma once
#include "MoaStateMachine.h"

class ConfigState : public MoaState {
    MoaStateMachine& _moaMachine;
public:
    ConfigState(MoaStateMachine& moaMachine, MoaDevicesManager& devices);
    void onEnter() override;
    void buttonClick(ControlCommand command) override;
    void overcurrentDetected(ControlCommand command) override;
    void temperatureCrossedLimit(ControlCommand command) override;
    void batteryLevelCrossedLimit(ControlCommand command) override;
    void timerExpired(ControlCommand command) override;
};
```

ConfigState.cpp:
- onEnter(): enterConfigMode(), startOTA(), log
- buttonClick(): STOP long press → stopOTA(), exitConfigMode(), → InitState
- Safety events: stopOTA(), exitConfigMode(), → respective error state

---

## Key Design Decisions

1. WiFi only in ConfigState — no RF during normal operation
2. Keep 8.5dBm TX power — your antenna workaround
3. Proper teardown: WiFi.softAPdisconnect(true) + WiFi.mode(WIFI_OFF)
4. Safety events exit ConfigState immediately (shut down WiFi first)
5. Throttle buttons ignored in ConfigState

---

## Implementation Order

1. Add stop() to MoaOTAManager
2. Add MoaOTAManager& to MoaDevicesManager
3. Add exitConfigMode() to MoaDevicesManager/MoaLedControl
4. Create ConfigState class
5. Register ConfigState in MoaStateMachine
6. Wire InitState very long press → ConfigState
7. Update OtaTask — remove begin() call
8. Update MoaMainUnit constructor
9. Build and verify

---

## Future Extensions

- Webserver in ConfigState
- NVS config read/write
- BLE alternative to WiFi

*Plan updated: ConfigState with 8.5dBm workaround preserved*

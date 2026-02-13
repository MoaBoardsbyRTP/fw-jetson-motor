# ConfigManager Plan

**Date:** 2026-02-13  
**Branch:** config-manager  
**Goal:** Persist user-tunable settings in NVS with compile-time defaults as fallback. Provide a `ConfigManager` class that loads from NVS on boot, applies values to each device, and can save changes from the webserver.

---

## Settings Inventory

All settings fall back to their `Constants.h` default if NVS is empty or corrupted.

### 1. Surfing Timers (throttle duration per speed)

| Setting | NVS Key | Type | Default | Constant |
|---------|---------|------|---------|----------|
| 25% throttle timeout | `esc_t25` | `uint32_t` (ms) | 240000 | `ESC_25_TIME` |
| 50% throttle timeout | `esc_t50` | `uint32_t` (ms) | 180000 | `ESC_50_TIME` |
| 75% throttle timeout | `esc_t75` | `uint32_t` (ms) | 90000 | `ESC_75_TIME` |
| 100% throttle timeout | `esc_t100` | `uint32_t` (ms) | 15000 | `ESC_100_TIME` |
| 75% step-down from 100% | `esc_t75_100` | `uint32_t` (ms) | 45000 | `ESC_75_TIME_100` |

### 2. Throttle Percentages (speed modes)

| Setting | NVS Key | Type | Default | Constant |
|---------|---------|------|---------|----------|
| Eco mode throttle | `esc_eco` | `uint8_t` (%) | 25 | `ESC_ECO_MODE` |
| Paddle mode throttle | `esc_paddle` | `uint8_t` (%) | 50 | `ESC_PADDLE_MODE` |
| Breaking zone throttle | `esc_break` | `uint8_t` (%) | 75 | `ESC_BREAKING_MODE` |
| Full throttle | `esc_full` | `uint8_t` (%) | 100 | `ESC_FULL_THROTTLE_MODE` |
| ESC ramp rate | `esc_ramp` | `float` (%/s) | 200.0 | `ESC_RAMP_RATE` |

### 3. Battery Thresholds

| Setting | NVS Key | Type | Default | Constant |
|---------|---------|------|---------|----------|
| High threshold | `batt_high` | `float` (V) | 21.5 | `BATT_THRESHOLD_HIGH` |
| Medium threshold | `batt_med` | `float` (V) | 20.0 | `BATT_THRESHOLD_MEDIUM` |
| Low threshold | `batt_low` | `float` (V) | 19.5 | `BATT_THRESHOLD_LOW` |
| Hysteresis | `batt_hyst` | `float` (V) | 0.2 | `BATT_HYSTERESIS` |

### 4. Temperature Thresholds

| Setting | NVS Key | Type | Default | Constant |
|---------|---------|------|---------|----------|
| Target (warning) threshold | `temp_tgt` | `float` (°C) | 78.0 | `TEMP_THRESHOLD_TARGET` |
| Hysteresis | `temp_hyst` | `float` (°C) | 13.0 | `TEMP_HYSTERESIS` |

### 5. Current Thresholds

| Setting | NVS Key | Type | Default | Constant |
|---------|---------|------|---------|----------|
| Overcurrent threshold | `curr_oc` | `float` (A) | 150.0 | `CURRENT_THRESHOLD_OVERCURRENT` |
| Reverse overcurrent threshold | `curr_rev` | `float` (A) | -150.0 | `CURRENT_THRESHOLD_REVERSE` |
| Hysteresis | `curr_hyst` | `float` (A) | 5.0 | `CURRENT_HYSTERESIS` |

**Total: 20 settings**

---

## Class Design

### `ConfigManager`

```
Location: include/Helpers/ConfigManager.h
          src/Helpers/ConfigManager.cpp
NVS Namespace: "moa_config"
```

#### Public API

```cpp
class ConfigManager {
public:
    ConfigManager();

    /// Load all settings from NVS (fallback to Constants.h defaults)
    void begin();

    /// Apply loaded settings to all device instances
    void applyTo(MoaBattControl& batt, MoaCurrentControl& current,
                 MoaTempControl& temp, ESCController& esc);

    /// Save all current settings to NVS
    bool save();

    /// Reset all settings to Constants.h defaults and save
    void resetToDefaults();

    // --- Surfing Timers ---
    uint32_t escTime25;
    uint32_t escTime50;
    uint32_t escTime75;
    uint32_t escTime100;
    uint32_t escTime75From100;

    // --- Throttle Percentages ---
    uint8_t escEcoMode;
    uint8_t escPaddleMode;
    uint8_t escBreakingMode;
    uint8_t escFullThrottle;
    float escRampRate;

    // --- Battery ---
    float battHigh;
    float battMedium;
    float battLow;
    float battHysteresis;

    // --- Temperature ---
    float tempTarget;
    float tempHysteresis;

    // --- Current ---
    float currentOvercurrent;
    float currentReverse;
    float currentHysteresis;
};
```

#### Key Behaviors

1. **`begin()`** — Opens NVS namespace `"moa_config"`, reads each key. If a key is missing (`ESP_ERR_NVS_NOT_FOUND`) or corrupted, uses the `Constants.h` default. Logs each loaded value.

2. **`applyTo(...)`** — Calls the existing setter methods on each device:
   - `batt.setHighThreshold(battHigh)`, `batt.setLowThreshold(battLow)`, `batt.setHysteresis(battHysteresis)`
   - `current.setOvercurrentThreshold(currentOvercurrent)`, `current.setReverseOvercurrentThreshold(currentReverse)`, `current.setHysteresis(currentHysteresis)`
   - `temp.setTargetTemp(tempTarget)`, `temp.setHysteresis(tempHysteresis)`
   - `esc.setRampRate(escRampRate)`

3. **`save()`** — Writes all 20 settings to NVS. Returns false if any write fails.

4. **`resetToDefaults()`** — Sets all members to `Constants.h` values, then calls `save()`.

---

## Integration Points

### Boot Sequence (MoaMainUnit)

Current flow in `MoaMainUnit::applyConfiguration()` reads directly from `Constants.h`. This changes to:

```
Before:  Constants.h → applyConfiguration() → device setters
After:   NVS (fallback Constants.h) → ConfigManager.begin() → ConfigManager.applyTo() → device setters
```

`MoaMainUnit` owns a `ConfigManager _config` member. In `begin()`:
```cpp
_config.begin();                    // Load from NVS
_config.applyTo(_battControl, _currentControl, _tempControl, _escController);
```

### Runtime (Utils.h)

`escThrottleLevel()` and `escThrottleTimeout()` currently return compile-time constants. They need access to ConfigManager values. Two options:

**Option A (recommended):** Make `MoaDevicesManager` hold a pointer/reference to `ConfigManager` and use its values directly in `engageThrottle()` and `handleThrottleStepDown()` instead of calling the static `Utils.h` functions.

**Option B:** Pass ConfigManager reference to the utility functions.

### Webserver (Future ConfigState)

The webserver will:
1. `GET /api/config` → Return all settings as JSON from ConfigManager members
2. `POST /api/config` → Parse JSON, update ConfigManager members, call `save()`, then `applyTo()` to hot-reload
3. `POST /api/config/reset` → Call `resetToDefaults()`, then `applyTo()`

---

## Implementation Steps

1. **Create `ConfigManager.h`** — Class declaration with all 20 public members
2. **Create `ConfigManager.cpp`** — `begin()`, `save()`, `resetToDefaults()`, `applyTo()`
3. **Add `ConfigManager` to `MoaMainUnit`** — Replace `applyConfiguration()` internals
4. **Refactor `Utils.h` / `MoaDevicesManager`** — Use ConfigManager values for throttle levels and timeouts instead of compile-time constants
5. **Compile and verify** — Ensure boot behavior is identical (NVS empty → all defaults)

---

## NVS Notes

- **Namespace:** `"moa_config"` (max 15 chars)
- **Keys:** All ≤ 15 chars (NVS limit)
- **Library:** `<Preferences.h>` (Arduino ESP32 wrapper around `nvs_flash`)
- **Partition:** Uses default `nvs` partition (already in ESP32 partition table)
- **Wear:** NVS has its own wear leveling; safe for occasional config writes
- **Corruption recovery:** If `Preferences::begin()` fails, all values stay at Constants.h defaults — system runs normally

---

## File Changes Summary

| File | Change |
|------|--------|
| `include/Helpers/ConfigManager.h` | **NEW** — Class declaration |
| `src/Helpers/ConfigManager.cpp` | **NEW** — Implementation |
| `src/Helpers/MoaMainUnit.cpp` | Modify `applyConfiguration()` to use ConfigManager |
| `include/Helpers/MoaMainUnit.h` | Add `ConfigManager _config` member |
| `src/Helpers/MoaDevicesManager.cpp` | Use ConfigManager values for throttle levels/timeouts |
| `include/Helpers/MoaDevicesManager.h` | Add ConfigManager reference/pointer |
| `include/Helpers/Utils.h` | May be removed or simplified |

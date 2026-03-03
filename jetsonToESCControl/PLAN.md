# Moa ESC Controller — Project Plan

**Last updated:** 2026-03-03

---

## Completed Milestones

### ✅ Phase 1: Core Hardware Abstraction
- All sensor producers (temperature, battery, current) with averaging, hysteresis, event generation
- MCP23018 I2C expander with mutex-protected access, hardware reset, interrupt-driven buttons
- LED output with blink patterns, wave animations, config mode indication
- ESC PWM control with ramped throttle transitions
- Flash logging (LittleFS circular buffer, 128 entries, JSON export)
- Stats aggregator for telemetry
- FreeRTOS task infrastructure

### ✅ Phase 2: State Machine Logic
- 7-state machine: Init, Idle, Surfing, OverHeating, OverCurrent, BatteryLow, Config
- Multi-stage button press (short, long 5s, very long 10s)
- Cross-safety transitions between error states
- Timer-based throttle timeouts with step-down from 100% → 75%
- SurfingState: only BATT_STOP forces throttle disengage (BATT_LOW is warning-only)

### ✅ Phase 3: Configuration & Refinement
- **ConfigManager** — 21 NVS-backed settings with `Constants.h` fallback
  - Surfing timers, throttle duty cycles, battery/temp/current thresholds, ESC ramp rate
  - `begin()` loads from NVS, `applyTo()` pushes to devices, `save()` persists, `resetToDefaults()` restores
- **UartCli** — Serial CLI (115200 baud) for `get/set/dump/save/apply/reset`
  - See `UART_CLI.md` for full key reference
- **Battery 4-level monitoring** — HIGH/MEDIUM/LOW/STOP thresholds
  - Downward transition debounce (300ms confirmation windows) to filter transient voltage dips from current peaks
  - Only BATT_STOP triggers throttle stop in SurfingState

---

### ✅ Phase 4: OTA Firmware Updates
- `MoaOTAManager` — WiFi **STA mode** (connects to user's router, not AP)
- WiFi credentials (`wifi_ssid`, `wifi_pass`, `ota_host`) stored in ConfigManager — NVS-backed, CLI-configurable
- `ConfigState` — Entered from InitState via very long press STOP (10s). Long press STOP exits back to Init.
- `OtaTask` — FreeRTOS task (priority 1, 50ms period) polling `ArduinoOTA.handle()`
- Connection timeout (15s), RSSI logging, empty-SSID guard
- LED config mode indication (all LEDs blink 300ms)
- OTA workflow: `set wifi_ssid <ssid>` → `set wifi_pass <pass>` → `save` → very long press STOP → flash via `platformio run -t upload --upload-port <IP>`

---

## Future

### ⏳ Phase 5: BLE Control
- GATT server for button simulation and throttle control
- BLETask with event queue integration

### ⏳ Phase 6: Web Configuration
- AsyncWebServer in ConfigState alongside OTA
- REST API: `GET /api/config`, `POST /api/config`, `POST /api/config/reset`
- HTML config page for browser-based tuning
- Hot-reload via `ConfigManager::applyTo()`

### ⏳ Backlog
- Serial telemetry streaming
- Watchdog timer integration
- Power consumption optimization
- Production test mode

---

## Key Documents

| File | Description |
|------|-------------|
| `ARCHITECTURE.md` | Full architecture, state machine, file structure, OTA bug analysis |
| `CONFIG_MANAGER_PLAN.md` | ConfigManager design (21 settings, NVS, API) |
| `UART_CLI.md` | UART CLI command reference and Python integration |
| `PLAN.md` | This file — project progress and roadmap |

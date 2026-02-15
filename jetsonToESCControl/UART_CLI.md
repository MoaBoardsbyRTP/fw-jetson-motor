# UART CLI — Serial Command Interface

**Baud rate:** 115200  
**Line ending:** `\n` (LF) or `\r\n` (CRLF)  
**Echo:** Enabled (characters are echoed back)  
**Prompt:** `> `

---

## Commands

| Command | Description |
|---------|-------------|
| `get <key>` | Read a single setting |
| `get all` | Read all settings (alias for `dump`) |
| `set <key> <value>` | Write a setting (in-memory only until `save`) |
| `dump` | Print all settings grouped by category |
| `save` | Persist current settings to NVS flash |
| `apply` | Hot-reload settings to devices (no reboot needed) |
| `reset` | Restore all settings to compile-time defaults, save, and apply |
| `help` | Show command and key reference |

---

## Settings Keys

### Surfing Timers (milliseconds)

| Key | Description | Default |
|-----|-------------|---------|
| `esc_t25` | Duration at 25% throttle | 240000 |
| `esc_t50` | Duration at 50% throttle | 180000 |
| `esc_t75` | Duration at 75% throttle | 90000 |
| `esc_t100` | Duration at 100% throttle | 15000 |
| `esc_t75_100` | Duration at 75% after stepping down from 100% | 45000 |

### Throttle Percentages

| Key | Description | Default | Unit |
|-----|-------------|---------|------|
| `esc_eco` | Eco mode throttle | 25 | % |
| `esc_paddle` | Paddle out mode throttle | 50 | % |
| `esc_break` | Breaking zone mode throttle | 75 | % |
| `esc_full` | Full throttle | 100 | % |
| `esc_ramp` | Throttle ramp rate | 200.0 | %/s |

### Battery Thresholds (Volts)

| Key | Description | Default |
|-----|-------------|---------|
| `batt_high` | High battery threshold | 21.5 |
| `batt_med` | Medium battery threshold | 20.0 |
| `batt_low` | Low battery threshold | 19.5 |
| `batt_hyst` | Hysteresis | 0.2 |

### Temperature Thresholds (°C)

| Key | Description | Default |
|-----|-------------|---------|
| `temp_tgt` | Warning threshold | 78.0 |
| `temp_hyst` | Hysteresis | 13.0 |

### Current Thresholds (Amps)

| Key | Description | Default |
|-----|-------------|---------|
| `curr_oc` | Overcurrent threshold | 150.0 |
| `curr_rev` | Reverse overcurrent threshold | -150.0 |
| `curr_hyst` | Hysteresis | 5.0 |

---

## Typical Workflow

### Fine-tuning a value

```
> set esc_eco 30
OK: esc_eco = 30 %
> apply
OK: Settings applied to devices
> save
OK: Settings saved to NVS
```

### Inspect current config

```
> dump
--- Surfing Timers (ms) ---
  esc_t25      = 240000 ms
  esc_t50      = 180000 ms
  ...
--- Throttle Percentages ---
  esc_eco      = 25 %
  ...
```

### Reset to factory defaults

```
> reset
OK: Reset to defaults, saved, and applied
```

---

## Important Notes

- **`set` is in-memory only** — changes are lost on reboot unless you call `save`.
- **`apply` pushes to devices immediately** — no reboot required. This lets you test a value before committing it.
- **`save` writes to NVS** — persists across reboots. Only call after you're happy with the value.
- **`reset`** restores all 20 settings to their `Constants.h` compile-time defaults, saves to NVS, and applies to devices in one step.
- Throttle percentages are clamped to 0–100 on `set`.

---

## Python Integration

The protocol is simple line-based ASCII over serial. A Python app can use `pyserial`:

```python
import serial
import time

class MoaCli:
    def __init__(self, port='/dev/ttyACM0', baud=115200):
        self.ser = serial.Serial(port, baud, timeout=1)
        time.sleep(2)  # wait for ESP32 boot
        self.ser.reset_input_buffer()

    def send(self, cmd: str) -> str:
        self.ser.write(f"{cmd}\n".encode())
        time.sleep(0.1)
        return self.ser.read(self.ser.in_waiting).decode()

    def get(self, key: str) -> str:
        return self.send(f"get {key}")

    def set(self, key: str, value) -> str:
        return self.send(f"set {key} {value}")

    def save(self) -> str:
        return self.send("save")

    def apply(self) -> str:
        return self.send("apply")

    def dump(self) -> str:
        return self.send("dump")

    def reset(self) -> str:
        return self.send("reset")

# Usage:
cli = MoaCli('/dev/ttyACM0')
print(cli.dump())
cli.set('esc_eco', 30)
cli.apply()
cli.save()
```

### Parsing responses

- **Success:** Lines starting with `OK:` 
- **Error:** Lines starting with `ERR:`
- **Data:** Lines with format `  key          = value unit`

A simple parser for data lines:

```python
def parse_setting(line: str) -> tuple:
    """Parse '  esc_eco      = 30 %' into ('esc_eco', '30', '%')"""
    parts = line.strip().split('=')
    if len(parts) != 2:
        return None
    key = parts[0].strip()
    val_unit = parts[1].strip().split()
    value = val_unit[0] if val_unit else ''
    unit = val_unit[1] if len(val_unit) > 1 else ''
    return (key, value, unit)
```

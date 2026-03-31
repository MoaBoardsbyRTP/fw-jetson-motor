#!/usr/bin/env python3
import argparse
import json
import re
import sys
import time
from pathlib import Path

LINE_RE = re.compile(r"^\s*([a-z0-9_]+)\s*=\s*(.+?)\s*$")

KEY_MAP = {
    "esc_t25": "escTime25",
    "esc_t50": "escTime50",
    "esc_t75": "escTime75",
    "esc_t100": "escTime100",
    "esc_t_after": "escTimeAfterFullThrottle",
    "esc_t_after_full": "escTimeAfterFullThrottle",
    "esc_t75_100": "escTimeAfterFullThrottle",
    "esc_eco": "escEcoMode",
    "esc_paddle": "escPaddleMode",
    "esc_break": "escBreakingMode",
    "esc_full": "escFullThrottle",
    "esc_after": "escAfterFullThrottle",
    "esc_after_full": "escAfterFullThrottle",
    "esc_ramp": "escRampRate",
    "batt_high": "battHigh",
    "batt_med": "battMedium",
    "batt_low": "battLow",
    "batt_stop": "battStop",
    "batt_hyst": "battHysteresis",
    "temp_tgt": "tempTarget",
    "temp_hyst": "tempHysteresis",
    "curr_oc": "currentOvercurrent",
    "curr_rev": "currentReverse",
    "curr_hyst": "currentHysteresis",
    "wifi_ssid": "wifiSsid",
    "wifi_pass": "wifiPassword",
    "ota_host": "otaHostname",
}

STRING_KEYS = {"wifi_ssid", "wifi_pass", "ota_host"}
REQUIRED_FIELDS = {
    "escTime25",
    "escTime50",
    "escTime75",
    "escTime100",
    "escTimeAfterFullThrottle",
    "escEcoMode",
    "escPaddleMode",
    "escBreakingMode",
    "escFullThrottle",
    "escAfterFullThrottle",
    "escRampRate",
    "battHigh",
    "battMedium",
    "battLow",
    "battStop",
    "battHysteresis",
    "tempTarget",
    "tempHysteresis",
    "currentOvercurrent",
    "currentReverse",
    "currentHysteresis",
}


def parse_numeric(rhs: str):
    token = rhs.strip().split()[0]
    if any(ch in token for ch in [".", "e", "E"]):
        return float(token)
    return int(token)


def parse_dump_text(raw_text: str) -> dict:
    config = {}

    for line in raw_text.splitlines():
        match = LINE_RE.match(line)
        if not match:
            continue

        raw_key, rhs = match.group(1), match.group(2)
        mapped = KEY_MAP.get(raw_key)
        if not mapped:
            continue

        if raw_key in STRING_KEYS:
            value = rhs.strip()
        else:
            try:
                value = parse_numeric(rhs)
            except (ValueError, IndexError):
                continue

        config[mapped] = value

    return config


def read_from_serial(port: str, baud: int, timeout_s: float, settle_s: float) -> str:
    try:
        import serial  # type: ignore
    except ImportError as exc:
        raise RuntimeError(
            "pyserial is required for --port mode. Install with: pip install pyserial"
        ) from exc

    ser = serial.Serial(port=port, baudrate=baud, timeout=0.2)
    try:
        time.sleep(settle_s)
        ser.reset_input_buffer()
        ser.write(b"dump\n")

        start = time.time()
        chunks = []
        while time.time() - start < timeout_s:
            waiting = ser.in_waiting
            if waiting:
                chunks.append(ser.read(waiting).decode(errors="ignore"))
            else:
                time.sleep(0.05)

        return "".join(chunks)
    finally:
        ser.close()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Export ConfigManager snapshot JSON from UART CLI dump output"
    )
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--input-file", help="Path to a text file containing UART dump output")
    source.add_argument("--port", help="Serial port (e.g., /dev/ttyACM0)")

    parser.add_argument("--baud", type=int, default=115200, help="Baudrate for serial mode")
    parser.add_argument("--timeout", type=float, default=2.0, help="Seconds to capture after sending dump")
    parser.add_argument("--settle", type=float, default=1.5, help="Serial settle time before capture")
    parser.add_argument(
        "--output",
        default="deployment_registry/current_config_snapshot.json",
        help="Output JSON snapshot path",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Fail if any required field is missing",
    )

    args = parser.parse_args()

    if args.input_file:
        raw_text = Path(args.input_file).read_text(encoding="utf-8")
    else:
        raw_text = read_from_serial(args.port, args.baud, args.timeout, args.settle)

    config = parse_dump_text(raw_text)

    missing = sorted(REQUIRED_FIELDS - set(config.keys()))
    if missing and args.strict:
        print("ERR: Missing required fields:", ", ".join(missing), file=sys.stderr)
        return 2

    out_path = Path(args.output).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(config, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"Exported snapshot: {out_path}")
    print(f"Captured fields:  {len(config)}")
    if missing:
        print("Missing fields:   " + ", ".join(missing))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
import argparse
import csv
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


def iso_now_utc() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def canonical_json_bytes(payload: object) -> bytes:
    return json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")


def sha256_hex(data: bytes) -> str:
    h = hashlib.sha256()
    h.update(data)
    return h.hexdigest()


def build_deployment_id(timestamp_utc: str, device_id: str) -> str:
    compact_ts = timestamp_utc.replace("-", "").replace(":", "").replace("T", "_").replace("Z", "")
    return f"{compact_ts}_{device_id}"


def ensure_registry_header(path: Path, header: list[str]) -> None:
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(header)


def append_registry_row(path: Path, row: dict[str, str], header: list[str]) -> None:
    with path.open("a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=header)
        writer.writerow(row)


def main() -> int:
    parser = argparse.ArgumentParser(description="Append firmware deployment data to deployment_registry/registry.csv")
    parser.add_argument("--device-id", required=True, help="Stable device ID (e.g., esp32c3-001)")
    parser.add_argument("--board-serial-number", required=True, help="Physical serial label on board")
    parser.add_argument("--firmware-version", required=True, help="Firmware version/tag")
    parser.add_argument("--firmware-git-commit", required=True, help="Git commit hash used for build")
    parser.add_argument("--client-name", required=True, help="Client/customer name")
    parser.add_argument("--client-site", default="", help="Optional site/location")
    parser.add_argument("--flashed-by", required=True, help="Operator name")
    parser.add_argument("--status", default="active", help="Deployment status")
    parser.add_argument("--notes", default="", help="Optional notes")
    parser.add_argument("--config-file", required=True, help="Path to JSON config snapshot")

    args = parser.parse_args()

    registry_root = Path(__file__).resolve().parent
    registry_csv = registry_root / "registry.csv"
    snapshots_root = registry_root / "config_snapshots"

    config_path = Path(args.config_file).resolve()
    if not config_path.exists():
        raise FileNotFoundError(f"Config file not found: {config_path}")

    config_payload = json.loads(config_path.read_text(encoding="utf-8"))
    config_bytes = canonical_json_bytes(config_payload)
    config_hash = sha256_hex(config_bytes)

    deployment_timestamp_utc = iso_now_utc()
    deployment_id = build_deployment_id(deployment_timestamp_utc, args.device_id)

    snapshot_dir = snapshots_root / args.device_id
    snapshot_dir.mkdir(parents=True, exist_ok=True)
    snapshot_path = snapshot_dir / f"{deployment_id}.json"
    snapshot_path.write_text(json.dumps(config_payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    header = [
        "deployment_id",
        "deployment_timestamp_utc",
        "device_id",
        "board_serial_number",
        "firmware_version",
        "firmware_git_commit",
        "client_name",
        "client_site",
        "flashed_by",
        "config_sha256",
        "config_snapshot_path",
        "status",
        "notes",
    ]

    ensure_registry_header(registry_csv, header)

    row = {
        "deployment_id": deployment_id,
        "deployment_timestamp_utc": deployment_timestamp_utc,
        "device_id": args.device_id,
        "board_serial_number": args.board_serial_number,
        "firmware_version": args.firmware_version,
        "firmware_git_commit": args.firmware_git_commit,
        "client_name": args.client_name,
        "client_site": args.client_site,
        "flashed_by": args.flashed_by,
        "config_sha256": config_hash,
        "config_snapshot_path": str(snapshot_path.relative_to(registry_root)),
        "status": args.status,
        "notes": args.notes,
    }

    append_registry_row(registry_csv, row, header)

    print("Added deployment entry")
    print(f"deployment_id: {deployment_id}")
    print(f"registry_csv:  {registry_csv}")
    print(f"snapshot:      {snapshot_path}")
    print(f"config_sha256: {config_hash}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# Deployment Registry

Simple local registry to track where each firmware build was deployed and with which flash settings.

## Files

- `registry.csv`: Main index (one row per deployment event).
- `config_snapshot_template.json`: Template for captured device configuration.
- `config_snapshots/`: Auto-created folder with immutable config snapshots.
- `export_config_from_uart.py`: Exports config snapshot JSON from UART `dump` output.
- `register_device.py`: Helper script to append a deployment entry.

## Quick Start

1. Export a JSON snapshot from a connected board:

```bash
python3 deployment_registry/export_config_from_uart.py \
  --port /dev/ttyACM0 \
  --output deployment_registry/current_config_snapshot.json \
  --strict
```

2. Register the deployment:

```bash
python3 deployment_registry/register_device.py \
  --device-id esp32c3-001 \
  --board-serial-number SN-0001 \
  --firmware-version v1.4.2 \
  --firmware-git-commit a1b2c3d \
  --client-name Acme \
  --client-site Barcelona \
  --flashed-by Oscar \
  --config-file deployment_registry/current_config_snapshot.json
```

This will:
- Compute a deterministic SHA256 of the config.
- Store an immutable snapshot at `deployment_registry/config_snapshots/<device_id>/<deployment_id>.json`.
- Append one row to `deployment_registry/registry.csv`.

## Export Options

- Serial mode (auto-send `dump`): `--port /dev/ttyACM0`
- File mode (parse saved monitor output):

```bash
python3 deployment_registry/export_config_from_uart.py \
  --input-file /tmp/moa_dump.txt \
  --output deployment_registry/current_config_snapshot.json
```

## Suggested Process

- Keep this folder in your private repo.
- After each flash/deploy, run `register_device.py` once.
- Optionally mirror this folder to cloud storage (Drive/Dropbox/OneDrive) for backup.

## Notes

- The registry is append-only by convention.
- If settings are changed on an existing board, add a new deployment entry instead of editing old rows.

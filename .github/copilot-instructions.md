# AstroPowerBox repository instructions

## Scope and commands

The active MicroPython application is rooted in this repository; run its
commands from the repository root. Archived predecessor projects live under
`legacy/` and are not part of the active application.

```bash
# Install CPython test dependencies
python -m pip install pytest -r backend/requirements-cpython.txt

# Run all CPython backend tests
./scripts/test-backend-local

# Run one backend test or test file
pytest -q backend/tests/test_config.py::test_config_loads_defaults_when_storage_is_empty
pytest -q backend/tests/test_main_routes.py

# Build the React frontend (copies Bootswatch CSS as part of the build)
(cd frontend && npm ci && npm run build)

# Run the frontend against the host in ASTROPOWERBOX_HOST
(cd frontend && ASTROPOWERBOX_HOST=<device-host> npm run dev)

# Run MicroPython tests on a connected device
./scripts/test-backend-mpy
./scripts/test-backend-mpy --skip-upload

# Deploy plain Python sources for iteration, optionally rebuilding/uploading the UI
./scripts/deploy-dev [--frontend] <board_name>

# Create and deploy a production filesystem bundle
./scripts/build-prod-bundle [--frontend] <board_name>
./scripts/deploy-prod-bundle <board_name>
```

`mpremote` is required for device deployment and on-device tests; `mpy-cross` is also required for production bundles unless `--no-mpy` is passed. Valid board names map to `backend/config_files/pinout_<board_name>.json`. CI runs the CPython suite; hardware-in-the-loop tests run only on a self-hosted ESP32 runner.

## Architecture

- `backend/main.py` is the Microdot entry point. It creates the `Board`, exposes configuration and static-asset routes, and broadcasts pin changes over SSE at `/api/events` with the `pins` event.
- `backend/board.py` owns runtime composition: persistent configuration, Wi-Fi manager, pinout selection, GPIO instances, status LED behavior, and the normalized pin-state snapshot published to listeners.
- `backend/board_compat.py` selects platform implementations at import time. CPython uses JSON-backed configuration and optional simulator GPIO/Wi-Fi selected by `SIMULATOR_GPIO=1` and `SIMULATOR_WIFI=1`; ESP32 MicroPython uses NVS, `machine`, and `network`. Keep shared business logic independent of those implementations and add platform behavior under `backend/boards/<platform>/`.
- `backend/protocols/` defines the interfaces shared by platform adapters. GPIO output implementations must preserve the callback API (`on_level_changed` for digital, `on_duty_changed` for PWM), because `Board` uses those callbacks to update SSE state.
- Pinout JSON is runtime configuration, not merely documentation. `Board` selects compatible `pinout*.json` files using hardware/variant metadata, persists a selected filename in configuration, and requires restart when the newly selected file differs from the loaded one. Production bundling copies all pinout files to the device root.
- The React UI in `frontend/` uses Redux Toolkit slices. `features/app/api.jsx` centralizes backend calls; `App.jsx` subscribes to the `pins` SSE stream and translates normalized pin objects into the PWM UI state. When changing routes, response shapes, or event names, update both sides and `backend/tests/test_frontend_backend_contract.py`.

## Repository conventions

- Backend modules use top-level imports relative to `backend/` (for example, `from board import Board`), matching MicroPython's deployed filesystem layout. Tests add `backend/` to `sys.path` in `backend/tests/conftest.py`.
- Store persisted configuration through the `Config`/`ConfigStorage` abstractions. CPython persists to `config_files/config.json`; ESP32 persists blobs in the `apb_config` NVS namespace. Configuration writes are explicit through `POST /api/config/write`, except pinout selection, which persists immediately.
- Treat PWM duty as a normalized float from `0.0` to `1.0`; platform GPIO adapters clamp values and emit callbacks only on actual changes. Digital outputs expose equivalent `duty` semantics through their protocol.
- Pin state payloads use `{id, role, kind, on}` and include `duty` for `kind: "pwm"`. Keep this shape and the `pins` event stable for the frontend SSE consumer.
- Frontend production assets are built into `frontend/build/`, then gzip-compressed and uploaded under device `/static/`; backend static routes always serve compressed `.gz` assets. Do not point device code at the uncompressed frontend build.

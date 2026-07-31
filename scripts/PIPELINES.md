# MicroPython ESP32 Pipelines

This project now includes two deployment pipelines:

1. Development pipeline: fast iteration with plain .py files.
2. Production pipeline: reproducible bundle with optional .mpy compilation.

## Development

Use the wrapper script:

```bash
./scripts/deploy-dev <board_name>
```

Options:

```bash
./scripts/deploy-dev --frontend --repl <board_name>
./scripts/deploy-dev --no-reset <board_name>
```

Behavior:

1. Uses the existing `./deploy` flow.
2. Uploads backend source files directly.
3. Keeps debugging simple and update cycle short.
4. Excludes `backend/tests` and `backend/tests_mpy` from normal deployment.

## Production

### 1) Build a bundle

```bash
./scripts/build-prod-bundle <board_name>
```

Options:

```bash
./scripts/build-prod-bundle --frontend <board_name>
./scripts/build-prod-bundle --no-mpy <board_name>
./scripts/build-prod-bundle --out-dir dist/prod <board_name>
```

Output layout:

```text
dist/prod/<board_name>/
  fs/
  manifest.json
  files.txt
  checksums.sha256
```

Bundle behavior:

1. Copies backend Python files into `fs/` using device path layout.
2. Generates `board_vars.py` from `backend/board_vars.py.envsubst`.
3. Copies all `pinout*.json` files to the filesystem root for runtime selection.
4. Compiles modules to `.mpy` unless `--no-mpy` is set.
5. Optionally includes pre-gzipped frontend static assets with `--frontend`.
6. Excludes `backend/tests` and `backend/tests_mpy` from production bundles.

### 2) Deploy a bundle

```bash
./scripts/deploy-prod-bundle <board_name>
```

Options:

```bash
./scripts/deploy-prod-bundle --bundle-dir dist/prod/<board_name>/fs <board_name>
./scripts/deploy-prod-bundle --no-reset <board_name>
./scripts/deploy-prod-bundle --repl <board_name>
```

Behavior:

1. Uploads every file from the bundle `fs/` directory.
2. Creates directories recursively on the device.
3. Resets MCU by default.

## Recommended Usage

1. Day-to-day coding: `deploy-dev`.
2. Pre-release validation: `build-prod-bundle --frontend` then `deploy-prod-bundle`.
3. Keep pinout and runtime config external for easy field changes.

## Testing (Local + ESP32)

You can run tests in both environments with dedicated scripts.

### Local tests (CPython)

```bash
./scripts/test-backend-local
```

Notes:

1. Uses pytest against `backend/tests`.
2. Focuses on logic that can run without ESP32 hardware.

### On-device tests (MicroPython)

```bash
./scripts/test-backend-mpy
```

Notes:

1. Uploads `backend/tests_mpy` and executes `tests_mpy/run_tests.py`.
2. Runs a lightweight test harness using plain assertions.
3. Use `--skip-upload` to re-run quickly without copying files again.
4. Resets the MCU after testing so the deployed application resumes and the
   serial REPL is returned to a known state.
5. On ESP32 hardware, validates the MicroPython runtime. When the application
   pinout is deployed (as it is in HIL CI), it also briefly pulses the
   configured PWM status LED and checks the GPIO adapter's duty-change
   callbacks.
6. Includes Wi-Fi HIL tests:
    - Station connect test uses `APB_TEST_WIFI_SSID` and optional
       `APB_TEST_WIFI_PSK`. If SSID is missing, the station-connect test fails.
    - AP fallback test attempts a non-existent station with hardcoded values
       `BadSSID` / `BadPSK`, then verifies fallback to AP mode.

### Suggested split

1. Keep pure logic tests in both suites (`backend/tests` and `backend/tests_mpy`).
2. Keep hardware-specific tests only on the ESP32 side.
3. Keep API contract tests local (faster and richer assertions).

## GitHub Workflows

The repository now includes CI workflows in .github/workflows:

1. backend-tests.yml
2. prod-bundle.yml
3. micropython-hil-tests.yml

### Backend Tests (CPython)

Workflow: backend-tests.yml

Behavior:

1. Runs on push, pull request, and manual dispatch.
2. Installs pytest and backend CPython requirements.
3. Executes scripts/test-backend-local.

### Production Bundle Artifacts

Workflow: prod-bundle.yml

Behavior:

1. Runs on manual dispatch and on version tags (v*).
2. Builds production bundles for supported ESP32 board profiles.
3. Uploads bundle artifacts from dist/prod/<board>/.
4. Uses --no-mpy for portability in hosted CI.

### MicroPython Tests in CI

Workflow: micropython-hil-tests.yml

Behavior:

1. Runs on pushes to `main` that change `backend/` and by manual dispatch.
2. Runs on a self-hosted runner labeled self-hosted and esp32.
3. Executes scripts/test-backend-mpy against a physically connected board.
4. Uses `esp32_wroom_v1` for push-triggered runs; manual dispatch can select a
   different board.
5. Keeps at most one pending run per branch when the self-hosted runner is
   unavailable, and limits execution to 20 minutes once a runner accepts it.

Notes about emulation:

1. There is no complete ESP32 hardware emulation in standard GitHub hosted runners.
2. MicroPython unix-port style emulation can validate pure logic only, not GPIO, timing, or board peripherals.
3. For real confidence, prefer hardware-in-the-loop tests (this workflow).

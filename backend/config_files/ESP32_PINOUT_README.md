# ESP32 Pinout Configuration Files

These JSON files document the hardware pin assignments for different ESP32 boards used in the AstroPowerBox project. They map logical pin functions to physical GPIO numbers.

## Configuration Structure

Each pinout file contains:

### `I2C` object
I2C bus pins for sensors, ADCs, and other peripherals:
- `SDA`: Serial Data pin
- `SCL`: Serial Clock pin

### `SPI` object (optional)
SPI bus for SD card or other high-speed peripherals:
- `SCK`: Serial Clock
- `MISO`: Master In, Slave Out
- `MOSI`: Master Out, Slave In
- `SS`: Slave Select

### `status_led`
GPIO pin for status LED indicator

### `pwm_outputs` array
Array of PWM-controlled outputs:
- `name`: Logical name (PWM0, PWM1, etc.)
- `pin`: GPIO pin number for PWM signal
- `thermistor_pin`: GPIO pin for thermistor analog input (-1 if not available)
- `type`: "Heater" (with temperature control) or "Output" (simple PWM)

### `fan_pwm`
GPIO pin for fan control (PWM)

### `buttons` array
User input buttons:
- `name`: Logical name (BTN_USER_1, etc.)
- `pin`: GPIO pin number

### `power_delivery` object (optional)
USB Power Delivery controller pins (CH224K):
- `CH224K_CFG1`, `CH224K_CFG2`, `CH224K_CFG3`: Configuration pins

## Available Configurations

### pinout_esp32_wroom_v1.json
**ESP32-WROOM-32 (Rev 1)** — Full-featured production board
- 6 PWM outputs (2 with thermistor support)
- SPI interface
- USB PD controller
- Recommended for high-power applications

### pinout_esp32_lolin_c3_mini.json
**LOLIN C3 Mini (ESP32-C3)** — Compact development board
- 3 PWM outputs (2 with thermistor support)
- Minimal GPIO footprint
- Good for space-constrained designs

### pinout_esp32_lolin_s2_mini.json
**LOLIN S2 Mini (ESP32-S2)** — Single-core variant
- 3 PWM outputs with full thermistor support
- Lower power consumption
- Single-core CPU

### pinout_esp32_template.json
**Template** — Use as a starting point for custom boards

## Usage in MicroPython Code

These files are referenced by the MicroPython ESP32 GPIO implementation:

```python
import json

# Load configuration
with open('config_files/pinout_esp32_lolin_c3_mini.json') as f:
    config = json.load(f)

# Access pin mappings
pwm0_pin = config['pinout']['pwm_outputs'][0]['pin']
i2c_sda = config['pinout']['I2C']['SDA']
thermistor_pin = config['pinout']['pwm_outputs'][0]['thermistor_pin']
```

## Custom Board Configuration

To create a configuration for your ESP32 board:

1. Identify GPIO pin assignments from your board's datasheet
2. Copy `pinout_esp32_template.json`
3. Fill in your pin numbers for each function
4. Test with actual hardware
5. Save in this directory with name `pinout_esp32_<boardname>.json`

## Notes

- These configurations document the **intended** pin assignments for AstroPowerBox
- Actual MicroPython code loads pin numbers from these files at runtime
- Multiple pins of the same type (e.g., PWM0, PWM1) are indexed in arrays
- Thermistor pins use GPIO analog input (ADC) capability
- Not all ESP32 boards have the same capabilities; choose the configuration matching your hardware

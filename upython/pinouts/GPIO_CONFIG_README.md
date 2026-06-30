# GPIO Configuration Guide

## Overview

The CPython GPIO implementation uses lgpio for direct hardware GPIO control. Pin mappings are defined in JSON configuration files, allowing easy adaptation to different hardware platforms without code changes.

## GPIO Format

GPIO pins are referenced using the format: `GPIO<chip>_<port><line>`

- **chip**: GPIO chip number (0-4 typically)
- **port**: Letter A-D representing port groups
- **line**: Pin number within the port (0-31)

### Port to Line Mapping

Each port encompasses 8 lines, so the actual line offset is calculated as:
- Port A: lines 0-7
- Port B: lines 8-15
- Port C: lines 16-23
- Port D: lines 24-31

**Examples:**
- `GPIO0_A4` = Chip 0, Port A, line 4 → actual line 4
- `GPIO3_C2` = Chip 3, Port C, line 2 → actual line 18 (16 + 2)
- `GPIO1_B3` = Chip 1, Port B, line 3 → actual line 11 (8 + 3)

## Configuration File Structure

Each GPIO pinout JSON file defines:

### `pinout` object
Maps logical pin names to GPIO format strings:
```json
"pinout": {
  "PWM0": "GPIO0_A12",
  "BTN0": "GPIO0_A2",
  "LED_STATUS": "GPIO0_B5"
}
```

### `adc` object
Configures I2C-connected ADC pins (ADS1115):
```json
"adc": {
  "ANALOG_IN0": {
    "i2c_bus": 1,
    "i2c_addr": "0x48",
    "channel": 0
  }
}
```

- **i2c_bus**: Linux I2C bus number (usually 1 or 2)
- **i2c_addr**: I2C address in hex (default ADS1115 is 0x48)
- **channel**: ADC channel 0-3

## Platform-Specific Configurations

### Raspberry Pi 4/5 (`gpio_pinout_rpi4.json`, `gpio_pinout_rpi5.json`)

- **GPIO Chip**: Single chip (GPIO0)
- **I2C Bus**: 1 (GPIO pins 2,3)
- **Port Mapping**: A=BCM0-7, B=BCM8-15, C=BCM16-23, D=BCM24-31
- **Example**: BCM pin 4 → GPIO0_A4

### Orange Pi 5 Ultra (`gpio_pinout_opi5_ultra.json`)

- **GPIO Chips**: GPIO0-GPIO4 (5 chips)
- **I2C Bus**: 2 (recommended for ADC)
- **SoC**: Rockchip RK3588
- **Port Mapping**: Standard A-D per chip
- **Common Chips**: GPIO1 (buttons), GPIO3 (PWM/relays), GPIO4 (LEDs)

## Usage

### Setting the Configuration

Define the GPIO configuration path via environment variable:

```bash
export GPIO_CONFIG_PATH=pinouts/gpio_pinout_rpi5.json
python backend/main.py
```

Default path if not set: `pinouts/gpio_pinout.json`

### In Code

Pins are instantiated by their logical name from the configuration:

```python
from boards.cpython.gpio import ButtonPin, PWMOutputPin

# Automatically looks up pin mapping from config
button = ButtonPin('BTN0')
pwm_led = PWMOutputPin('PWM0')

# Use the pin
button.on_level_changed(lambda pressed: print(f"Pressed: {pressed}"))
pwm_led.duty = 0.5  # 50% duty cycle
```

## GPIO Classes

### ButtonPin
- **Property**: `value` → bool (True if pressed/active-low)
- **Method**: `on_level_changed(callback)` → registers callback for edge detection

### DigitalOutputPin
- **Property**: `on` → bool (get/set HIGH/LOW)
- **Property**: `duty` → float (0.0/1.0 only)
- **Property**: `is_pwm` → False

### PWMOutputPin
- **Property**: `duty` → float (0.0 to 1.0)
- **Property**: `on` → bool (True if duty > 0)
- **Property**: `is_pwm` → True
- **Frequency**: 1000 Hz default, configurable at init

### AnalogInputPin
- **Property**: `value` → float (voltage 0.0-4.096V for ADS1115)
- Configured via ADC section in JSON

## Creating a Custom Configuration

1. Copy `gpio_pinout_template.json`
2. Identify GPIO mappings for your hardware:
   - Use `gpioinfo` command to list available GPIO chips/lines
   - Cross-reference with hardware datasheets
3. Define logical pin names in `pinout` section
4. Add any I2C ADC pins to `adc` section
5. Set `GPIO_CONFIG_PATH` to your file and test

## Example: Custom Board

```json
{
  "hardware": "My Custom Board",
  "soc": "Broadcom BCM2835",
  "pinout": {
    "MOTOR_PWM": "GPIO0_D12",
    "MOTOR_DIR": "GPIO0_D13",
    "SENSOR_IN": "GPIO0_A4",
    "STATUS_LED": "GPIO0_B5"
  },
  "adc": {
    "VOLTAGE_SENSE": {
      "i2c_bus": 1,
      "i2c_addr": "0x48",
      "channel": 0
    }
  }
}
```

Then in code:
```python
from boards.cpython.gpio import PWMOutputPin, DigitalOutputPin

motor = PWMOutputPin('MOTOR_PWM', frequency=50)  # Servo frequency
direction = DigitalOutputPin('MOTOR_DIR')
led = DigitalOutputPin('STATUS_LED')

motor.duty = 0.5
direction.on = True
led.on = True
```

## Troubleshooting

### "Pin not found in configuration"
- Verify the pin name matches exactly (case-sensitive)
- Check GPIO_CONFIG_PATH points to correct file
- Ensure JSON is valid JSON

### "lgpio module not available"
- Install: `pip install lgpio`
- Requires lgpio daemon running: `sudo lgpiod`

### "ADS1x15 module not available"
- Install: `pip install adafruit-circuitpython-ads1x15`
- For analog pins only; digital GPIO works without it

### GPIO Permission Denied
- lgpio requires root or proper permissions
- Run with sudo or add user to gpio group

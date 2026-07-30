#!/usr/bin/env python3
import lgpio
from gpioparser import gpio2line

class OPiDevice:
#    _chip_handle = None

    def __init__(self, chip, line):
        # chip_line format "chip:offset" e.g. "3:18"
        self.chip_num, self.offset = chip, line
#        if OPiDevice._chip_handle is None:
#            OPiDevice._chip_handle = lgpio.gpiochip_open(self.chip_num)
#        self.handle = OPiDevice._chip_handle
        self.handle = lgpio.gpiochip_open(self.chip_num)

class LED(OPiDevice):
    def __init__(self, chip, line):
        super().__init__(chip, line)
        lgpio.gpio_claim_output(self.handle, self.offset)

    def on(self):
        lgpio.gpio_write(self.handle, self.offset, 1)

    def off(self):
        lgpio.gpio_write(self.handle, self.offset, 0)

    @property
    def value(self):
        return lgpio.gpio_read(self.handle, self.offset)

    @value.setter
    def value(self, val):
        lgpio.gpio_write(self.handle, self.offset, 1 if val else 0)

class PWMLED(OPiDevice):
    def __init__(self, chip, line, frequency=1000):
        super().__init__(chip, line)
        self.freq = frequency
        lgpio.gpio_claim_output(self.handle, self.offset)

    def on(self):
        self.value = 1.0

    def off(self):
        self.value = 0.0

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, val):
        # lgpio takes 0-100 for duty cycle
        self._value = max(0.0, min(1.0, val))
        lgpio.tx_pwm(self.handle, self.offset, self.freq, self._value * 100)

class Button(OPiDevice):
    GPIO_IS_KERNEL         = 1 << 0
    GPIO_IS_OUT            = 1 << 1
    GPIO_IS_ACTIVE_LOW     = 1 << 2
    GPIO_IS_OPEN_DRAIN     = 1 << 3
    GPIO_IS_OPEN_SOURCE    = 1 << 4
    GPIO_IS_BIAS_PULL_UP   = 1 << 5
    GPIO_IS_BIAS_PULL_DOWN = 1 << 6
    GPIO_IS_BIAS_DISABLE   = 1 << 7
    GPIO_IS_LG_INPUT       = 1 << 8
    GPIO_IS_LG_OUTPUT      = 1 << 9
    GPIO_IS_LG_ALERT       = 1 << 10
    GPIO_IS_LG_GROUP       = 1 << 11
    GPIO_LINE_FLAGS_MASK = (
        GPIO_IS_ACTIVE_LOW | GPIO_IS_OPEN_DRAIN | GPIO_IS_OPEN_SOURCE |
        GPIO_IS_BIAS_PULL_UP | GPIO_IS_BIAS_PULL_DOWN | GPIO_IS_BIAS_DISABLE)

    def __init__(self, chip, line):
        super().__init__(chip, line)
        # lgpio.gpio_claim_input(self.handle, self.offset, lgpio.SET_PULL_UP)
        # 2. Set hardware-timed debounce (prevents multiple triggers)
        # lgpio.set_debounce_period(self.handle, self.offset, debounce_ms)
        
        # 3. Placeholders for user functions
        self.when_pressed = None
        self.when_released = None
        
        # 4. Start the internal callback listener for both edges
        claim_alert_result = lgpio.gpio_claim_alert(
            self.handle, self.offset, lgpio.BOTH_EDGES,
            lgpio.gpio_get_mode(self.handle, self.offset) &
            self.GPIO_LINE_FLAGS_MASK)
        print(f'claim_alert_result={claim_alert_result}')
        self._cb = lgpio.callback(self.handle, self.offset, lgpio.BOTH_EDGES, self._internal_callback)

    def _internal_callback(self, chip, gpio, level, tick):
        print(f'callback: {chip}, {gpio}, {level}, {tick}')
        """
        level: 0 = falling edge (pressed if pulled up), 1 = rising edge (released)
        """
        if level == 0 and self.when_pressed:
            self.when_pressed()
        elif level == 1 and self.when_released:
            self.when_released()

    def close(self):
        """Clean up the callback and chip handle"""
        if hasattr(self, '_cb'):
            self._cb.cancel()

    @property
    def is_pressed(self):
        # Returns True if pressed (assuming pull-up to Ground)
        return lgpio.gpio_read(self.handle, self.offset) == 0

if __name__ == '__main__':
    import sys
    import time
    from opi5_ultra_gpio_map import OPI5_ULTRA_GPIO_PINS 
    name = sys.argv[1]
    if name.startswith('GPIO'):
        chip, line = gpio2line(name)
    else:
        chip, line = gpio2line(OPI5_ULTRA_GPIO_PINS[name])
    print(f'{name} resolved to chip {chip}, line {line}')
    led = PWMLED(chip, line)
    if len(sys.argv) > 2:
        led.value = float(sys.argv[2])
        if len(sys.argv) > 3:
            time.sleep(int(sys.argv[3]))
            led.value = 0
        sys.exit(0)

    print('setting led {led} to 0')
    led.value = 0
    time.sleep(5)
    print(f'setting led {led} to 0.5')
    led.value = 0.5
    time.sleep(5)
    print(f'setting led {led} to 1')
    led.value = 1
    time.sleep(5)


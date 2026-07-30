import re
import typing
import json
import os

try:
    import lgpio
except ImportError:
    lgpio = None

try:
    import ADS1x15
    _ads1115_available = True
except ImportError:
    _ads1115_available = False


class PinConfig:
    """Manages pin configuration loaded from JSON files"""
    
    def __init__(self, config_path: str):
        """Load pin configuration from JSON file"""
        self.config = {}
        self.adc_config = {}
        
        if os.path.exists(config_path):
            try:
                with open(config_path, 'r') as f:
                    data = json.load(f)
                    self.config = data.get('pinout', {})
                    self.adc_config = data.get('adc', {})
            except Exception as e:
                raise RuntimeError(f'Failed to load pin configuration from {config_path}: {e}')
        else:
            raise RuntimeError(f'Pin configuration file not found: {config_path}')
    
    def get_gpio_pin(self, pin_name: str) -> str:
        """Get GPIO format string for a pin name (e.g., 'PWM1' -> 'GPIO3_C2')"""
        if pin_name not in self.config:
            raise ValueError(f'Pin {pin_name} not found in configuration')
        return self.config[pin_name]
    
    def get_adc_config(self, pin_name: str) -> dict:
        """Get ADC configuration for a pin"""
        if pin_name not in self.adc_config:
            raise ValueError(f'ADC pin {pin_name} not found in configuration')
        return self.adc_config[pin_name]


# Global pin configuration (initialized on first use)
_pin_config: PinConfig | None = None


def _get_pin_config() -> PinConfig:
    """Get or initialize global pin configuration"""
    global _pin_config
    if _pin_config is None:
        # Determine config path from environment or use default
        config_path = os.environ.get('GPIO_CONFIG_PATH', 'config_files/gpio_pinout.json')
        _pin_config = PinConfig(config_path)
    return _pin_config


def _parse_gpio_pin(gpio_format: str) -> tuple[int, int]:
    """
    Parse GPIO pin name format: GPIO<chip>_<port><pin>
    Examples: GPIO0_A5, GPIO1_B3
    Returns: (chip_number, line_offset)
    """
    pattern = r'^GPIO(\d)_([A-D])(\d+)$'
    match = re.match(pattern, gpio_format.upper())
    if not match:
        raise ValueError(f'Invalid GPIO format: {gpio_format}. Expected format: GPIO<chip>_<port><pin>, e.g., GPIO0_A5')
    
    chip = int(match.group(1))
    port = match.group(2)
    pin = int(match.group(3))
    
    # Convert port letter (A-D) to base offset: A=0, B=8, C=16, D=24
    port_offset = (ord(port) - ord('A')) * 8
    line = port_offset + pin
    
    return chip, line


class ButtonPin:
    """Button input pin with edge detection callback support"""
    
    def __init__(self, pin_name: str):
        if lgpio is None:
            raise RuntimeError('lgpio module not available')
        
        self.pin_name = pin_name
        config = _get_pin_config()
        gpio_format = config.get_gpio_pin(pin_name)
        self.chip, self.line = _parse_gpio_pin(gpio_format)
        self.handle = lgpio.gpiochip_open(self.chip)
        self._callback = None
        self._lgpio_callback = None
        
        # Claim the GPIO line as input with alert capability
        lgpio.gpio_claim_alert(
            self.handle, self.line, lgpio.BOTH_EDGES,
            lgpio.gpio_get_mode(self.handle, self.line)
        )
    
    @property
    def value(self) -> bool:
        """Returns True if button is pressed (active low), False if released"""
        # Assuming active-low: 0 = pressed, 1 = released
        return lgpio.gpio_read(self.handle, self.line) == 0
    
    def on_level_changed(self, callback: typing.Callable[[bool], None]) -> None:
        """
        Register callback for level changes.
        Callback receives bool: True when pressed (falling edge), False when released (rising edge)
        """
        self._callback = callback
        
        # Set up lgpio callback
        if self._lgpio_callback is not None:
            self._lgpio_callback.cancel()
        
        def _internal_callback(chip: int, gpio: int, level: int, tick: int):
            # level: 0 = falling edge (pressed), 1 = rising edge (released)
            if self._callback:
                self._callback(level == 0)
        
        self._lgpio_callback = lgpio.callback(
            self.handle, self.line, lgpio.BOTH_EDGES, _internal_callback
        )
    
    def __del__(self):
        """Clean up callback and chip handle"""
        if self._lgpio_callback is not None:
            self._lgpio_callback.cancel()


class AnalogInputPin:
    """Analog input pin using ADS1115 ADC"""
    
    # Class-level cache for ADS1115 instances (shared across pins)
    _ads_instances: dict[tuple[int, int], 'ADS1x15.ADS1115'] = {}
    
    def __init__(self, pin_name: str):
        """
        Initialize analog input pin.
        
        Args:
            pin_name: Pin identifier in configuration (e.g., 'ANALOG_IN1')
        """
        if not _ads1115_available:
            raise RuntimeError('ADS1x15 module not available')
        
        self.pin_name = pin_name
        config = _get_pin_config()
        adc_config = config.get_adc_config(pin_name)
        
        self.i2c_bus = adc_config.get('i2c_bus', 2)
        self.i2c_addr = adc_config.get('i2c_addr', 0x48)
        self.adc_channel = adc_config.get('channel', 0)
        
        # Get or create ADS1115 instance
        key = (self.i2c_bus, self.i2c_addr)
        if key not in AnalogInputPin._ads_instances:
            ads = ADS1x15.ADS1115(self.i2c_bus, self.i2c_addr)
            ads.setGain(ads.PGA_4_096V)  # Set to 4.096V max for better resolution
            AnalogInputPin._ads_instances[key] = ads
        
        self.ads = AnalogInputPin._ads_instances[key]
        self._voltage_scale = self.ads.toVoltage()
    
    @property
    def value(self) -> float:
        """
        Read analog value from ADC.
        Returns voltage as float (0.0 to 4.096V for ADS1115 with PGA_4_096V)
        """
        raw = self.ads.readADC(self.adc_channel)
        return raw * self._voltage_scale


class DigitalOutputPin:
    """Digital output pin (on/off only)"""
    
    def __init__(self, pin_name: str):
        if lgpio is None:
            raise RuntimeError('lgpio module not available')
        
        self.pin_name = pin_name
        config = _get_pin_config()
        gpio_format = config.get_gpio_pin(pin_name)
        self.chip, self.line = _parse_gpio_pin(gpio_format)
        self.handle = lgpio.gpiochip_open(self.chip)
        self._value = False
        self._callbacks: list[typing.Callable[[bool], None]] = []
        
        # Claim as output
        lgpio.gpio_claim_output(self.handle, self.line)
        lgpio.gpio_write(self.handle, self.line, 0)
    
    @property
    def is_pwm(self) -> bool:
        """This is not a PWM pin"""
        return False
    
    @property
    def on(self) -> bool:
        """Returns True if pin is HIGH, False if LOW"""
        return self._value
    
    @on.setter
    def on(self, value: bool) -> None:
        """Set pin HIGH (True) or LOW (False)"""
        new_value = bool(value)
        if new_value == self._value:
            return
        self._value = new_value
        lgpio.gpio_write(self.handle, self.line, 1 if self._value else 0)
        for callback in self._callbacks:
            callback(self._value)

    def on_level_changed(self, callback: typing.Callable[[bool], None]) -> None:
        """Register callback for output level changes"""
        self._callbacks.append(callback)
    
    @property
    def duty(self) -> float:
        """Returns 1.0 if on, 0.0 if off"""
        return 1.0 if self._value else 0.0
    
    @duty.setter
    def duty(self, value: float) -> None:
        """Set on (True) if duty > 0, off (False) if duty == 0"""
        self.on = value > 0.0


class PWMOutputPin:
    """PWM output pin with variable duty cycle"""
    
    def __init__(self, pin_name: str, frequency: int = 1000):
        """
        Initialize PWM output pin.
        
        Args:
            pin_name: GPIO pin identifier from configuration
            frequency: PWM frequency in Hz (default: 1000)
        """
        if lgpio is None:
            raise RuntimeError('lgpio module not available')
        
        self.pin_name = pin_name
        self.frequency = frequency
        config = _get_pin_config()
        gpio_format = config.get_gpio_pin(pin_name)
        self.chip, self.line = _parse_gpio_pin(gpio_format)
        self.handle = lgpio.gpiochip_open(self.chip)
        self._duty = 0.0
        self._callbacks: list[typing.Callable[[float], None]] = []
        
        # Claim as output
        lgpio.gpio_claim_output(self.handle, self.line)
        lgpio.tx_pwm(self.handle, self.line, self.frequency, 0)
    
    @property
    def is_pwm(self) -> bool:
        """This is a PWM pin"""
        return True
    
    @property
    def duty(self) -> float:
        """Returns duty cycle (0.0 to 1.0)"""
        return self._duty
    
    @duty.setter
    def duty(self, value: float) -> None:
        """
        Set PWM duty cycle.
        
        Args:
            value: Duty cycle from 0.0 (off) to 1.0 (full on)
        """
        new_duty = max(0.0, min(1.0, value))
        if new_duty == self._duty:
            return
        self._duty = new_duty
        # lgpio tx_pwm expects duty as 0-100
        lgpio.tx_pwm(self.handle, self.line, self.frequency, self._duty * 100)
        for callback in self._callbacks:
            callback(self._duty)
    
    @property
    def on(self) -> bool:
        """Returns True if duty > 0"""
        return self._duty > 0.0
    
    @on.setter
    def on(self, value: bool) -> None:
        """Set full on (True) or off (False)"""
        self.duty = 1.0 if value else 0.0

    def on_duty_changed(self, callback: typing.Callable[[float], None]) -> None:
        """Register callback for duty cycle changes"""
        self._callbacks.append(callback)

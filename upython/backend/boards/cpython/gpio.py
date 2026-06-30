import re
import typing

try:
    import lgpio
except ImportError:
    lgpio = None

try:
    import ADS1x15
    _ads1115_available = True
except ImportError:
    _ads1115_available = False


def _parse_gpio_pin(pin_name: str) -> tuple[int, int]:
    """
    Parse GPIO pin name format: GPIO<chip>_<port><pin>
    Examples: GPIO0_A5, GPIO1_B3
    Returns: (chip_number, line_offset)
    """
    pattern = r'^GPIO(\d)_([A-D])(\d+)$'
    match = re.match(pattern, pin_name.upper())
    if not match:
        raise ValueError(f'Invalid GPIO pin name: {pin_name}. Expected format: GPIO<chip>_<port><pin>, e.g., GPIO0_A5')
    
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
        self.chip, self.line = _parse_gpio_pin(pin_name)
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
    
    def __init__(self, pin_name: str, i2c_bus: int = 2, i2c_addr: int = 0x48, adc_channel: int = 0):
        """
        Initialize analog input pin.
        
        Args:
            pin_name: Pin identifier (can be anything, used for logging)
            i2c_bus: I2C bus number for ADS1115 (default: 2)
            i2c_addr: I2C address of ADS1115 (default: 0x48)
            adc_channel: ADC channel 0-3 on the ADS1115 (default: 0)
        """
        if not _ads1115_available:
            raise RuntimeError('ADS1x15 module not available')
        
        self.pin_name = pin_name
        self.i2c_bus = i2c_bus
        self.i2c_addr = i2c_addr
        self.adc_channel = adc_channel
        
        # Get or create ADS1115 instance
        key = (i2c_bus, i2c_addr)
        if key not in AnalogInputPin._ads_instances:
            ads = ADS1x15.ADS1115(i2c_bus, i2c_addr)
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
        self.chip, self.line = _parse_gpio_pin(pin_name)
        self.handle = lgpio.gpiochip_open(self.chip)
        self._value = False
        
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
        self._value = bool(value)
        lgpio.gpio_write(self.handle, self.line, 1 if self._value else 0)
    
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
            pin_name: GPIO pin identifier
            frequency: PWM frequency in Hz (default: 1000)
        """
        if lgpio is None:
            raise RuntimeError('lgpio module not available')
        
        self.pin_name = pin_name
        self.frequency = frequency
        self.chip, self.line = _parse_gpio_pin(pin_name)
        self.handle = lgpio.gpiochip_open(self.chip)
        self._duty = 0.0
        
        # Claim as output
        lgpio.gpio_claim_output(self.handle, self.line)
    
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
        self._duty = max(0.0, min(1.0, value))
        # lgpio tx_pwm expects duty as 0-100
        lgpio.tx_pwm(self.handle, self.line, self.frequency, self._duty * 100)
    
    @property
    def on(self) -> bool:
        """Returns True if duty > 0"""
        return self._duty > 0.0
    
    @on.setter
    def on(self, value: bool) -> None:
        """Set full on (True) or off (False)"""
        self.duty = 1.0 if value else 0.0

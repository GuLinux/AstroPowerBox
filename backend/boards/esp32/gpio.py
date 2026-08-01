import protocols.gpio
from machine import Pin, PWM, ADC
from protocols.typing_compat import Callable

class GPIO(protocols.gpio.GPIO):
    def __init__(self, pin_name: str):
        print(f'Initialising {type(self).__name__} with pin {pin_name}')
        self.pin_name = pin_name


class ButtonPin(GPIO, protocols.gpio.ButtonPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self.pin = Pin(int(pin_name), Pin.IN, Pin.PULL_UP)
        self._callback = None

    @property
    def value(self) -> bool:
        """Returns True if button is pressed (active low), False if released"""
        return self.pin.value() == 0
    
    def on_level_changed(self, callback: Callable[[bool], None]) -> None:
        """Register callback for level changes. Callback receives bool (True=pressed, False=released)"""
        self._callback = callback
        
        def _internal_callback(pin):
            if self._callback:
                # pin.value() == 0 means pressed (active low)
                self._callback(pin.value() == 0)
        
        self.pin.irq(handler=_internal_callback, trigger=Pin.IRQ_FALLING | Pin.IRQ_RISING)

class AnalogInputPin(GPIO, protocols.gpio.AnalogInputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self.adc = ADC(Pin(int(pin_name)))
        # Attenuation 11dB allows measuring up to ~3.3V
        self.adc.atten(ADC.ATTN_11DB)

    @property
    def value(self) -> float:
        """Returns analog voltage (0.0 to ~3.3V for 11dB attenuation)"""
        # ADC returns 0-4095, convert to voltage (0-3.3V for 11dB attenuation)
        raw = self.adc.read()
        return (raw / 4095.0) * 3.3

class DigitalOutputPin(GPIO, protocols.gpio.DigitalOutputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self.pin = Pin(int(pin_name), Pin.OUT)
        self._value = False
        self._callbacks: list[Callable[[bool], None]] = []
        self.pin.value(0)

    @property
    def is_pwm(self) -> bool:
        """This is not a PWM pin"""
        return False

    @property
    def on(self) -> bool:
        return self._value

    @on.setter
    def on(self, on: bool) -> None:
        new_value = bool(on)
        if new_value == self._value:
            return
        self.pin.value(1 if new_value else 0)
        self._value = new_value
        for callback in self._callbacks:
            callback(self._value)

    def on_level_changed(self, callback: Callable[[bool], None]) -> None:
        self._callbacks.append(callback)

class PWMOutputPin(GPIO, protocols.gpio.PWMOutputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self.pin = PWM(Pin(int(pin_name)), freq=1000)
        self._duty = 0.0
        self._callbacks: list[Callable[[float], None]] = []
        self.pin.duty_u16(0)

    @property
    def is_pwm(self) -> bool:
        """This is a PWM pin"""
        return True

    @property
    def duty(self) -> float:
        #return self._duty
        return self.pin.duty_u16() / 65535.0

    @duty.setter
    def duty(self, duty: float) -> None:
        new_duty = max(0.0, min(1.0, duty))
        if new_duty == self._duty:
            return
        self._duty = new_duty
        self.pin.duty_u16(int(self._duty * 65535))
        for callback in self._callbacks:
            callback(self._duty)

    def on_duty_changed(self, callback: Callable[[float], None]) -> None:
        self._callbacks.append(callback)

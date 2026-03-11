import typing
import protocols.gpio
from machine import Pin, PWM

class GPIO(protocols.gpio.GPIO):
    def __init__(self, pin_name: str):
        print(f'Initialising {type(self).__name__} with pin {pin_name}')
        self.pin_name = pin_name


class ButtonPin(GPIO, protocols.gpio.ButtonPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)

    @property
    def value(self) -> bool:
        raise NotImplementedError()
    
    def on_level_changed(self, callback: typing.Callable[[bool], None]) -> None:
        raise NotImplementedError()

class AnalogInputPin(GPIO, protocols.gpio.AnalogInputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)

    @property
    def value(self) -> float:
        raise NotImplementedError()

class DigitalOutputPin(GPIO, protocols.gpio.DigitalOutputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self.pin = Pin(int(pin_name), Pin.OUT)
        self._value = False
        self.on = False

    @property
    def on(self) -> bool:
        return self._value

    @on.setter
    def on(self, on: bool) -> None:
        self.pin.value(1 if on else 0)
        self._value = on 

class PWMOutputPin(GPIO, protocols.gpio.PWMOutputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self.pin = PWM(Pin(int(pin_name)), freq=1000)
        self.duty = 0.0

    @property
    def duty(self) -> float:
        return self.pin.duty_u16() / 65535.0

    @duty.setter
    def duty(self, duty: float) -> None:
        self.pin.duty_u16(int(duty * 65535))

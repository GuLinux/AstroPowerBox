import typing
import protocols.gpio
import os

class GPIO(protocols.gpio.GPIO):
    def __init__(self, pin_name: str):
        print(f'Initialising {type(self).__name__} with pin {pin_name}')
        self.pin_name = pin_name
        self.pin_file = f'/tmp/AstroPowerBox-GPIOSimulator/{pin_name}'
        os.makedirs(os.path.dirname(self.pin_file), exist_ok=True)


    def _write(self, value: str):
        with open(self.pin_file, 'w') as f:
            f.write(value)


class ButtonPin(GPIO, protocols.gpio.ButtonPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self._pressed = False
        self._callback = None

    @property
    def value(self) -> bool:
        """Returns current button state: True if pressed, False if released"""
        return self._pressed
    
    def on_level_changed(self, callback: typing.Callable[[bool], None]) -> None:
        """Register callback for level changes. Callback receives bool (True=pressed, False=released)"""
        self._callback = callback
    
    def _trigger_callback(self, pressed: bool) -> None:
        """Internal method to trigger the callback (used for testing)"""
        self._pressed = pressed
        if self._callback:
            self._callback(pressed)

class AnalogInputPin(GPIO, protocols.gpio.AnalogInputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        # Default simulated value: 2.0V (middle of typical 0-4.096V range)
        self._value = 2.0

    @property
    def value(self) -> float:
        """Returns simulated voltage (default 2.0V, can be overridden via sim_value)"""
        return self._value
    
    def set_simulated_value(self, voltage: float) -> None:
        """Set the simulated voltage value (for testing)"""
        self._value = max(0.0, min(4.096, voltage))

class DigitalOutputPin(GPIO, protocols.gpio.DigitalOutputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self._value = False
        self._callbacks: list[typing.Callable[[bool], None]] = []
        self._write('False')

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
        self._write(str(new_value))
        self._value = new_value
        for callback in self._callbacks:
            callback(self._value)

    def on_level_changed(self, callback: typing.Callable[[bool], None]) -> None:
        self._callbacks.append(callback)

class PWMOutputPin(GPIO, protocols.gpio.PWMOutputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self._value = 0.0
        self._callbacks: list[typing.Callable[[float], None]] = []
        self._write('0.0')

    @property
    def is_pwm(self) -> bool:
        """This is a PWM pin"""
        return True

    @property
    def duty(self) -> float:
        return self._value

    @duty.setter
    def duty(self, duty: float) -> None:
        new_value = max(0.0, min(1.0, duty))
        if new_value == self._value:
            return
        self._value = new_value
        self._write(str(self._value))

        for callback in self._callbacks:
            callback(self._value)

    def on_duty_changed(self, callback: typing.Callable[[float], None]) -> None:
        self._callbacks.append(callback)


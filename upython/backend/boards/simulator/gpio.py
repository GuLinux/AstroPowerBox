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
        self._value = False

    @property
    def on(self) -> bool:
        return self._value

    @on.setter
    def on(self, on: bool) -> None:
        self._write(str(on))
        self._value = on 

class PWMOutputPin(GPIO, protocols.gpio.PWMOutputPin):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)
        self._value = 0.0

    @property
    def duty(self) -> float:
        return self._value

    @duty.setter
    def duty(self, duty: float) -> None:
        self._write(str(duty))


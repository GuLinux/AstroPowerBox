import typing

class GPIO(typing.Protocol):
    def __init__(self, pin_name: str):
        pass

class ButtonPin(GPIO, typing.Protocol):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)

    @property
    def value(self) -> bool:
        raise NotImplementedError()
    
    def on_level_changed(self, callback: typing.Callable[[bool], None]) -> None:
        raise NotImplementedError()

class AnalogInputPin(GPIO, typing.Protocol):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)

    @property
    def value(self) -> float:
        raise NotImplementedError()

class DigitalOutputPin(GPIO, typing.Protocol):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)

    @property
    def value(self) -> bool:
        raise NotImplementedError()

    @value.setter
    def value(self, value: bool) -> None:
        raise NotImplementedError()

    @property
    def is_pwm(self):
        return False


class PWMPin(GPIO, typing.Protocol):
    def __init__(self, pin_name: str):
        super().__init__(pin_name)

    @property
    def is_pwm(self):
        return True

    @property
    def duty(self) -> float:
        raise NotImplementedError()

    @duty.setter
    def duty(self, duty: float) -> None:
        raise NotImplementedError()

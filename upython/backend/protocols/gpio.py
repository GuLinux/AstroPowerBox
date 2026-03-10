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
    def __init__(self, pin_name: str) -> None:
        super().__init__(pin_name)

    @property
    def is_pwm(self) -> bool:
        return False
    
    @property
    def on(self) -> bool:
        raise NotImplementedError()
    
    @on.setter
    def on(self, on: bool) -> None:
        raise NotImplementedError()

    @property
    def duty(self) -> float:
        return 1.0 if self.on else 0.0
    @duty.setter
    def duty(self, duty: float) -> None:
        self.on = duty > 0

class PWMOutputPin(GPIO, typing.Protocol):
    def __init__(self, pin_name: str) -> None:
        super().__init__(pin_name)

    @property
    def is_pwm(self) -> bool:
        return True

    @property
    def duty(self) -> float:
        raise NotImplementedError()
    
    @duty.setter
    def duty(self, duty: float) -> None:
        raise NotImplementedError()

    @property
    def on(self) -> bool:
        return self.duty > 0
    @on.setter
    def on(self, on: bool) -> None:
        self.duty = 1 if on else 0

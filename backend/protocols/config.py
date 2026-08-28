from protocols.config_storage import ConfigStorage
from protocols.typing_compat import Protocol
from wifi import WiFi

class Config(Protocol):
    def __init__(self, storage: ConfigStorage):
        pass

    @property
    def ap(self) -> WiFi:
        raise NotImplementedError()

    @ap.setter
    def ap(self, ap: WiFi) -> None:
        raise NotImplementedError()

    @property
    def stations(self) -> list[WiFi]:
        raise NotImplementedError()

    @stations.setter
    def stations(self, stations: list[WiFi]) -> None:
        raise NotImplementedError() 

    @property
    def status_led_duty(self) -> float:
        raise NotImplementedError()
    
    @status_led_duty.setter
    def status_led_duty(self, duty: float) -> None:
        raise NotImplementedError()

    @property
    def fan_duty(self) -> float:
        raise NotImplementedError()

    @fan_duty.setter
    def fan_duty(self, duty: float) -> None:
        raise NotImplementedError()

    @property
    def pinout_file(self) -> str:
        raise NotImplementedError()

    @pinout_file.setter
    def pinout_file(self, pinout_file: str) -> None:
        raise NotImplementedError()

    @property
    def pwm_output_startup(self) -> dict[str, dict]:
        raise NotImplementedError()

    @pwm_output_startup.setter
    def pwm_output_startup(self, pwm_output_startup: dict[str, dict]) -> None:
        raise NotImplementedError()

    def load(self) -> None:
        raise NotImplementedError()

    def save(self) -> None:
        raise NotImplementedError()

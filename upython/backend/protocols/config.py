from protocols.config_storage import ConfigStorage
from typing import Protocol
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

    def load(self) -> None:
        raise NotImplementedError()

    def save(self) -> None:
        raise NotImplementedError()

from protocols.config_storage import ConfigStorage
from typing import Protocol


class WiFi(Protocol):
    def __init__(self, ssid: str, psk: str | None = None):
        pass

    @property
    def ssid(self) -> str:
        raise NotImplementedError()

    @ssid.setter
    def ssid(self, ssid: str) -> None:
        raise NotImplementedError()

    @property
    def psk(self) -> str | None:
        raise NotImplementedError()

    @psk.setter
    def psk(self, psk: str) -> None:
        raise NotImplementedError()

    @property
    def json(self) -> dict:
        raise NotImplementedError()

    @classmethod
    def from_json(cls, json: dict) -> WiFi:
        raise NotImplementedError()

    @classmethod
    def from_json_list(cls, json: list) -> list[WiFi]:
        raise NotImplementedError()

    @classmethod
    def to_json_list(cls, json: list) -> list[dict]:
        raise NotImplementedError()


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

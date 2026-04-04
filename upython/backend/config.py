from utils import check_required_keys
from protocols.config_storage import ConfigStorage
import protocols.config
from wifi import WiFi

class Config(protocols.config.Config):
    _status_led_duty: float
    _ap: WiFi
    _stations: list[WiFi]

    def __init__(self, storage: ConfigStorage):
        self.storage = storage
        self.load()

    @property
    def ap(self) -> WiFi:
        return self._ap

    @ap.setter
    def ap(self, ap: WiFi) -> None:
        self._ap = ap

    @property
    def stations(self) -> list[WiFi]:
        return self._stations

    @stations.setter
    def stations(self, stations: list[WiFi]) -> None:
        self._stations = stations

    @property
    def status_led_duty(self) -> float:
        return self._status_led_duty
    
    @status_led_duty.setter
    def status_led_duty(self, duty: float) -> None:
        self._status_led_duty = duty

    @property
    def json(self) -> dict:
        return {
            'ap': self.ap.json if self.ap else None,
            'stations': WiFi.to_json_list(self._stations),
            'statusLedDuty': self._status_led_duty,
        }

    def load(self) -> None:
        self._ap, self._stations = self.storage.load_wifi()
        self._status_led_duty = self.storage.load_float('stLedDuty', default=1) or 1.0

    def save(self) -> None:
        self.storage.save_wifi(self._stations, self._ap)
        self.storage.save_float('stLedDuty', self._status_led_duty)

    def __str__(self):
        return f'Config{self.json}'

    def __repr__(self):
        return self.__str__()
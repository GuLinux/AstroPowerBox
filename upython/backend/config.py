from utils import check_required_keys
from protocols.config_storage import ConfigStorage
import typing
import protocols.config


class WiFi:
    def __init__(self, ssid: str, psk: str | None = None):
        self._ssid = ssid
        self._psk = psk

    @property
    def ssid(self) -> str:
        return self._ssid

    @ssid.setter
    def ssid(self, ssid: str) -> None:
        self._ssid = ssid

    @property
    def psk(self) -> str | None:
        return self._psk

    @psk.setter
    def psk(self, psk: str) -> None:
        self._psk = psk

    @property
    def json(self) -> dict:
        return {
            'ssid': self.ssid,
            'psk': self.psk,
        }

    @classmethod
    def from_json(cls, json: dict) -> protocols.config.WiFi:
        check_required_keys(['ssid'], json)
        return WiFi(json['ssid'], json.get('psk', ''))

    @classmethod
    def from_json_list(cls, json: list) -> list[protocols.config.WiFi]:
        return [WiFi.from_json(wifi) for wifi in json]

    @classmethod
    def to_json_list(cls, json: list) -> list[dict]:
        return [wifi.json for wifi in json]

    def __str__(self):
        return f'WiFi{self.json}'

    def __repr__(self):
        return self.__str__()


class Config(protocols.config.Config):
    _status_led_duty: float
    _ap: protocols.config.WiFi
    _stations: list[protocols.config.WiFi]

    def __init__(self, storage: ConfigStorage):
        self.storage = storage
        self.load()

    @property
    def ap(self) -> protocols.config.WiFi:
        return self._ap

    @ap.setter
    def ap(self, ap: protocols.config.WiFi) -> None:
        self._ap = ap

    @property
    def stations(self) -> list[protocols.config.WiFi]:
        return self._stations

    @stations.setter
    def stations(self, stations: list[protocols.config.WiFi]) -> None:
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
        self._stations = self._read_wifi_stations_from_config()
        self._ap = self._read_wifi_ap_from_config()
        self._status_led_duty = self.storage.load_float('stLedDuty', default=1) or 1.0

    def save(self) -> None:
        self.storage.save_json('stations', WiFi.to_json_list(self._stations))
        self.storage.save_json('ap', self._ap.json)
        self.storage.save_float('stLedDuty', self._status_led_duty)

    def __str__(self):
        return f'Config{self.json}'

    def __repr__(self):
        return self.__str__()

    def _read_wifi_stations_from_config(self) -> list[protocols.config.WiFi]:
        stations_json = self.storage.load_json('stations', maximum_size=1500, default=[])
        if type(stations_json) is not list:
            raise ValueError('Stations config must be a JSON array')
        return WiFi.from_json_list(stations_json)

    def _read_wifi_ap_from_config(self) -> protocols.config.WiFi:
        ap_json = self.storage.load_json('ap', default=WiFi(ssid='AstroPowerBox', psk='astropowerbox').json)
        if type(ap_json) is not dict:
            raise ValueError('AP config must be a JSON object')
        return WiFi.from_json(ap_json)

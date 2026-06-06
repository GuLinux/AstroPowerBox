from __future__ import annotations # see https://stackoverflow.com/questions/33533148/how-do-i-type-hint-a-method-with-the-type-of-the-enclosing-class
from utils import check_required_keys


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
    def from_json(cls, json: dict) -> WiFi:
        check_required_keys(['ssid'], json)
        return WiFi(json['ssid'], json.get('psk', ''))

    @classmethod
    def from_json_list(cls, json: list) -> list[WiFi]:
        return [WiFi.from_json(wifi) for wifi in json]

    @classmethod
    def to_json_list(cls, json: list) -> list[dict]:
        return [wifi.json for wifi in json]

    def __str__(self):
        return f'WiFi{self.json}'

    def __repr__(self):
        return self.__str__()


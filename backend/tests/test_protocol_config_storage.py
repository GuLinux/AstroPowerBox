import pytest

from protocols.config_storage import ConfigStorage
from wifi import WiFi


class DummyStorage(ConfigStorage):
    def __init__(self):
        self.data = {}

    def load_str(self, key: str, maximum_size: int = 1024, default: str = '') -> str | None:
        return self.data.get(key, default)

    def load_int(self, key: str, default: int = 0) -> int | None:
        return self.data.get(key, default)

    def load_float(self, key: str, default: float = 0.0) -> float | None:
        return self.data.get(key, default)

    def load_json(self, key: str, maximum_size: int = 1024, default: dict | list = {}):
        return self.data.get(key, default)

    def save_str(self, key: str, value: str) -> None:
        self.data[key] = value

    def save_int(self, key: str, value: int) -> None:
        self.data[key] = value

    def save_float(self, key: str, value: float) -> None:
        self.data[key] = value

    def save_json(self, key: str, value: dict | list) -> None:
        self.data[key] = value


def test_save_wifi_serializes_to_stations_and_ap_keys():
    storage = DummyStorage()
    stations = [WiFi('Home', '1234'), WiFi('Mobile', '')]
    ap = WiFi('AstroPowerBox', 'astropowerbox')

    storage.save_wifi(stations, ap)

    assert storage.data['stations'] == [
        {'ssid': 'Home', 'psk': '1234'},
        {'ssid': 'Mobile', 'psk': ''},
    ]
    assert storage.data['ap'] == {'ssid': 'AstroPowerBox', 'psk': 'astropowerbox'}


def test_load_wifi_deserializes_to_wifi_objects():
    storage = DummyStorage()
    storage.data['stations'] = [
        {'ssid': 'Home', 'psk': '1234'},
        {'ssid': 'Mobile', 'psk': ''},
    ]
    storage.data['ap'] = {'ssid': 'AstroPowerBox', 'psk': 'astropowerbox'}

    ap, stations = storage.load_wifi()
    assert ap.ssid == 'AstroPowerBox'
    assert ap.psk == 'astropowerbox'
    assert [station.ssid for station in stations] == ['Home', 'Mobile']


def test_load_wifi_raises_when_stations_is_not_list():
    storage = DummyStorage()
    storage.data['stations'] = {'ssid': 'bad'}

    with pytest.raises(ValueError, match='Stations config must be a JSON array'):
        storage.load_wifi()


def test_load_wifi_raises_when_ap_is_not_object():
    storage = DummyStorage()
    storage.data['stations'] = []
    storage.data['ap'] = ['bad']

    with pytest.raises(ValueError, match='AP config must be a JSON object'):
        storage.load_wifi()

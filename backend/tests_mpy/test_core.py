from config import Config
from wifi import WiFi
from utils import check_required_key


class _MemoryStorage:
    def __init__(self):
        self.data = {}

    def load_str(self, key, maximum_size=1024, default=''):
        return self.data.get(key, default)

    def load_float(self, key, default=0.0):
        return self.data.get(key, default)

    def load_wifi(self):
        ap = self.data.get('ap', {'ssid': 'AstroPowerBox', 'psk': 'astropowerbox'})
        stations = self.data.get('stations', [])
        return WiFi.from_json(ap), WiFi.from_json_list(stations)

    def save_str(self, key, value):
        self.data[key] = value

    def save_float(self, key, value):
        self.data[key] = value

    def save_wifi(self, stations, ap):
        self.data['stations'] = WiFi.to_json_list(stations)
        self.data['ap'] = ap.json


def _assert(condition, message):
    if not condition:
        raise AssertionError(message)


def test_wifi_requires_ssid():
    try:
        WiFi.from_json({'psk': 'secret'})
    except KeyError:
        return
    raise AssertionError('WiFi.from_json should raise KeyError when ssid is missing')


def test_wifi_roundtrip():
    source = [WiFi('A', 'a123'), WiFi('B', '')]
    payload = WiFi.to_json_list(source)
    loaded = WiFi.from_json_list(payload)
    _assert(len(loaded) == 2, 'Expected two WiFi entries')
    _assert(loaded[0].ssid == 'A', 'Unexpected SSID for first WiFi')
    _assert(loaded[1].ssid == 'B', 'Unexpected SSID for second WiFi')


def test_check_required_key_raises():
    try:
        check_required_key('ssid', {})
    except KeyError:
        return
    raise AssertionError('check_required_key should raise when key is missing')


def test_config_roundtrip_persists_wifi_and_runtime_settings():
    storage = _MemoryStorage()
    config = Config(storage)
    config.ap = WiFi('Test AP', 'ap-secret')
    config.stations = [WiFi('Home', 'station-secret')]
    config.status_led_duty = 0.25
    config.pinout_file = 'pinout_esp32_wroom_v1.json'
    config.save()

    reloaded = Config(storage)
    _assert(reloaded.ap.ssid == 'Test AP', 'Access point did not persist')
    _assert(reloaded.stations[0].ssid == 'Home', 'Station did not persist')
    _assert(reloaded.status_led_duty == 0.25, 'Status LED duty did not persist')
    _assert(
        reloaded.pinout_file == 'pinout_esp32_wroom_v1.json',
        'Pinout selection did not persist',
    )

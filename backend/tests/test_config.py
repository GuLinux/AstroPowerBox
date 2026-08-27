from config import Config


class MemoryStorage:
    def __init__(self, initial=None):
        self.data = initial or {}

    def load_str(self, key, maximum_size=1024, default=''):
        return self.data.get(key, default)

    def load_int(self, key, default=0):
        return self.data.get(key, default)

    def load_float(self, key, default=0.0):
        return self.data.get(key, default)

    def load_json(self, key, maximum_size=1024, default=None):
        if default is None:
            default = {}
        return self.data.get(key, default)

    def save_str(self, key, value):
        self.data[key] = value

    def save_int(self, key, value):
        self.data[key] = value

    def save_float(self, key, value):
        self.data[key] = value

    def save_json(self, key, value):
        self.data[key] = value

    def save_wifi(self, stations, ap):
        from wifi import WiFi
        self.save_json('stations', WiFi.to_json_list(stations))
        self.save_json('ap', ap.json)

    def load_wifi(self):
        from wifi import WiFi

        stations_json = self.load_json('stations', default=[])
        if type(stations_json) is not list:
            raise ValueError('Stations config must be a JSON array')

        ap_json = self.load_json('ap', default=WiFi(ssid='AstroPowerBox', psk='astropowerbox').json)
        if type(ap_json) is not dict:
            raise ValueError('AP config must be a JSON object')

        return WiFi.from_json(ap_json), WiFi.from_json_list(stations_json)


def test_config_loads_defaults_when_storage_is_empty():
    cfg = Config(MemoryStorage())

    assert cfg.ap.ssid == 'AstroPowerBox'
    assert cfg.ap.psk == 'astropowerbox'
    assert cfg.stations == []
    assert cfg.status_led_duty == 1.0
    assert cfg.fan_duty == 1.0
    assert cfg.pinout_file == ''


def test_config_save_persists_pinout_and_status_led():
    storage = MemoryStorage()
    cfg = Config(storage)

    cfg.status_led_duty = 0.42
    cfg.fan_duty = 0.35
    cfg.pinout_file = 'pinout_esp32_c3.json'
    cfg.save()

    assert storage.data['stLedDuty'] == 0.42
    assert storage.data['fanDuty'] == 0.35
    assert storage.data['pinoutFile'] == 'pinout_esp32_c3.json'


def test_config_json_includes_pinout_file():
    cfg = Config(MemoryStorage())
    cfg.pinout_file = 'pinout_esp32_wroom_v1.json'
    cfg.fan_duty = 0.5

    payload = cfg.json
    assert payload['pinoutFile'] == 'pinout_esp32_wroom_v1.json'
    assert payload['fanDuty'] == 0.5

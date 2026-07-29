import asyncio
import importlib
import sys
import types


def _import_main(monkeypatch):
    microdot = types.ModuleType('microdot')

    class _Microdot:
        def route(self, *_args, **_kwargs):
            def decorator(func):
                return func

            return decorator

    def _send_file(*_args, **_kwargs):
        return None

    microdot.Microdot = _Microdot
    microdot.send_file = _send_file

    microdot_sse = types.ModuleType('microdot.sse')

    def _with_sse(func):
        return func

    microdot_sse.with_sse = _with_sse

    board_compat = types.ModuleType('board_compat')
    board_compat.asyncio = asyncio
    board_compat.server_port = 80
    board_compat.server_debug = False

    config_module = types.ModuleType('config')

    class _WiFi:
        def __init__(self, ssid, psk=''):
            self.ssid = ssid
            self.psk = psk

        @property
        def json(self):
            return {'ssid': self.ssid, 'psk': self.psk}

        @classmethod
        def from_json(cls, payload):
            return cls(payload.get('ssid', ''), payload.get('psk', ''))

        @classmethod
        def to_json_list(cls, stations):
            return [station.json for station in stations]

    config_module.WiFi = _WiFi

    board_module = types.ModuleType('board')

    class _ImportBoard:
        def __init__(self):
            self.config = types.SimpleNamespace(
                stations=[],
                ap=_WiFi('AstroPowerBox', 'astropowerbox'),
                status_led_duty=1.0,
                json={
                    'statusLedDuty': 1.0,
                    'pinoutFile': '',
                    'ap': {'ssid': 'AstroPowerBox', 'psk': 'astropowerbox'},
                    'stations': [],
                },
                save=lambda: None,
            )
            self.wifi_manager = types.SimpleNamespace(connect_stations=self._noop_async)

        async def _noop_async(self):
            return None

        def on_pin_update(self, _callback):
            return None

        def pin_status_snapshot(self):
            return {'pins': []}

        async def start(self):
            return None

        def get_pinout_selection(self):
            return {'configured': '', 'selected': '', 'selectedFile': '', 'restartRequired': False}

        def list_available_pinout_files(self):
            return []

        def set_pinout_file(self, _file_name):
            return {'configured': '', 'selected': '', 'selectedFile': '', 'restartRequired': False}

    board_module.Board = _ImportBoard

    monkeypatch.setitem(sys.modules, 'microdot', microdot)
    monkeypatch.setitem(sys.modules, 'microdot.sse', microdot_sse)
    monkeypatch.setitem(sys.modules, 'board_compat', board_compat)
    monkeypatch.setitem(sys.modules, 'config', config_module)
    monkeypatch.setitem(sys.modules, 'board', board_module)

    module = importlib.import_module('main')
    return importlib.reload(module)


class _Request:
    def __init__(self, payload=None):
        self.json = payload or {}
        self.method = 'POST'


class _FakeConfig:
    def __init__(self):
        self._status_led_duty = 1.0
        self._save_calls = 0

    @property
    def status_led_duty(self):
        return self._status_led_duty

    @status_led_duty.setter
    def status_led_duty(self, duty):
        self._status_led_duty = duty

    @property
    def json(self):
        return {
            'statusLedDuty': self._status_led_duty,
            'pinoutFile': 'pinout_esp32_c3.json',
            'ap': {'ssid': 'AstroPowerBox', 'psk': 'astropowerbox'},
            'stations': [],
        }

    def save(self):
        self._save_calls += 1


class _FakeBoard:
    def __init__(self):
        self.config = _FakeConfig()

    def get_pinout_selection(self):
        return {
            'configured': 'pinout_esp32_c3.json',
            'selected': '/pinout_esp32_c3.json',
            'selectedFile': 'pinout_esp32_c3.json',
            'restartRequired': False,
        }

    def list_available_pinout_files(self):
        return [
            {
                'file': 'pinout_esp32_c3.json',
                'path': '/pinout_esp32_c3.json',
                'hardware': 'esp32',
                'variant': 'c3',
                'board': 'generic',
                'hardwareName': 'ESP32-C3 Dev Board (Generic)',
            }
        ]

    def set_pinout_file(self, file_name):
        if file_name == 'bad.json':
            raise ValueError('Pinout file not found or incompatible: bad.json')
        return {
            'configured': file_name,
            'selected': '/pinout_esp32_c3.json',
            'selectedFile': 'pinout_esp32_c3.json',
            'restartRequired': file_name != 'pinout_esp32_c3.json',
        }


def test_get_pinout_files_returns_files_and_current(monkeypatch):
    main = _import_main(monkeypatch)
    main.board = _FakeBoard()

    payload = asyncio.run(main.get_pinout_files(_Request()))
    assert 'files' in payload
    assert 'current' in payload
    assert payload['files'][0]['file'] == 'pinout_esp32_c3.json'


def test_get_pinout_config_returns_selection(monkeypatch):
    main = _import_main(monkeypatch)
    main.board = _FakeBoard()

    payload = asyncio.run(main.get_pinout_config(_Request()))
    assert payload['configured'] == 'pinout_esp32_c3.json'


def test_set_pinout_config_success(monkeypatch):
    main = _import_main(monkeypatch)
    main.board = _FakeBoard()

    payload = asyncio.run(main.set_pinout_config(_Request({'file': 'pinout_esp32_c3.json'})))
    assert payload['configured'] == 'pinout_esp32_c3.json'
    assert payload['restartRequired'] is False


def test_set_pinout_config_validation_error(monkeypatch):
    main = _import_main(monkeypatch)
    main.board = _FakeBoard()

    payload, status = asyncio.run(main.set_pinout_config(_Request({'file': 'bad.json'})))
    assert status == 400
    assert 'error' in payload


def test_set_status_led_duty_returns_full_config_payload(monkeypatch):
    main = _import_main(monkeypatch)
    main.board = _FakeBoard()

    payload = asyncio.run(main.set_status_led_duty(_Request({'duty': 0.33})))
    assert payload['statusLedDuty'] == 0.33
    assert payload['pinoutFile'] == 'pinout_esp32_c3.json'


def test_write_config_triggers_save(monkeypatch):
    main = _import_main(monkeypatch)
    fake_board = _FakeBoard()
    main.board = fake_board

    payload = asyncio.run(main.write_config(_Request()))
    assert fake_board.config._save_calls == 1
    assert payload['statusLedDuty'] == 1.0

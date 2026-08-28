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
                fan_duty=1.0,
                json={
                    'statusLedDuty': 1.0,
                    'fanDuty': 1.0,
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

        def pin_event_snapshot(self):
            return {}

        def has_temperature_sensor(self, _pin_id):
            return False

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
        self._fan_duty = 1.0
        self._pwm_output_startup = {}
        self._save_calls = 0

    @property
    def status_led_duty(self):
        return self._status_led_duty

    @status_led_duty.setter
    def status_led_duty(self, duty):
        self._status_led_duty = duty

    @property
    def fan_duty(self):
        return self._fan_duty

    @fan_duty.setter
    def fan_duty(self, duty):
        self._fan_duty = duty

    @property
    def pwm_output_startup(self):
        return self._pwm_output_startup

    @pwm_output_startup.setter
    def pwm_output_startup(self, value):
        self._pwm_output_startup = value

    @property
    def json(self):
        return {
            'statusLedDuty': self._status_led_duty,
            'fanDuty': self._fan_duty,
            'pwmOutputStartup': self._pwm_output_startup,
            'pinoutFile': 'pinout_esp32_c3.json',
            'ap': {'ssid': 'AstroPowerBox', 'psk': 'astropowerbox'},
            'stations': [],
        }

    def save(self):
        self._save_calls += 1


class _FakeBoard:
    def __init__(self):
        self.config = _FakeConfig()
        self.last_set_fan_duty = None

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

    def set_fan_duty(self, duty):
        self.last_set_fan_duty = duty
        self.config.fan_duty = duty

    def set_pwm_output_startup(self, pin_id, profile, apply_at_startup):
        if apply_at_startup:
            self.config.pwm_output_startup[pin_id] = profile
        else:
            self.config.pwm_output_startup.pop(pin_id, None)


class _FakePWMOutputPin:
    def __init__(self, board_state, pin_id):
        self.is_pwm = True
        self._board_state = board_state
        self._pin_id = pin_id

    @property
    def duty(self):
        return float(self._board_state[self._pin_id].get('duty', 0.0))

    @duty.setter
    def duty(self, value):
        duty = max(0.0, min(1.0, float(value)))
        self._board_state[self._pin_id]['duty'] = duty
        self._board_state[self._pin_id]['on'] = duty > 0


class _FakePWMBoard:
    def __init__(self):
        self.config = _FakeConfig()
        self.pin_states = {
            'output_0': {
                'id': 'output_0',
                'role': 'output',
                'kind': 'pwm',
                'duty': 0.0,
                'on': False,
            }
        }
        self.output_pins = {
            'output_0': _FakePWMOutputPin(self.pin_states, 'output_0')
        }

    def pin_status_snapshot(self):
        return {'pins': list(self.pin_states.values())}

    def has_temperature_sensor(self, _pin_id):
        return False

    def set_pwm_output_startup(self, pin_id, profile, apply_at_startup):
        if apply_at_startup:
            self.config.pwm_output_startup[pin_id] = profile
        else:
            self.config.pwm_output_startup.pop(pin_id, None)


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


def test_set_fan_duty_returns_full_config_payload(monkeypatch):
    main = _import_main(monkeypatch)
    fake_board = _FakeBoard()
    main.board = fake_board

    payload = asyncio.run(main.set_fan_duty(_Request({'duty': 0.21})))
    assert fake_board.last_set_fan_duty == 0.21
    assert payload['fanDuty'] == 0.21
    assert payload['pinoutFile'] == 'pinout_esp32_c3.json'


def test_set_pwm_output_can_persist_apply_at_startup(monkeypatch):
    main = _import_main(monkeypatch)
    main.board = _FakePWMBoard()
    main.board.config = _FakeConfig()

    payload = asyncio.run(main.set_pwm_output(_Request({
        'index': 0,
        'mode': 'target_temperature',
        'duty': 0.6,
        'max_duty': 0.6,
        'min_duty': 0.2,
        'target_temperature': 22,
        'dewpoint_offset': 4,
        'ramp_offset': 1.5,
        'apply_at_startup': True,
    })))

    startup = main.board.config.pwm_output_startup['output_0']
    assert startup['mode'] == 'target_temperature'
    assert startup['max_duty'] == 0.6
    assert startup['min_duty'] == 0.2
    assert startup['target_temperature'] == 22.0
    assert startup['dewpoint_offset'] == 4.0
    assert startup['ramp_offset'] == 1.5
    assert payload['pwmOutputs'][0]['apply_at_startup'] is True
    assert payload['pwmOutputs'][0]['mode'] == 'target_temperature'


def test_set_pwm_output_can_clear_apply_at_startup(monkeypatch):
    main = _import_main(monkeypatch)
    main.board = _FakePWMBoard()
    main.board.config = _FakeConfig()
    main.board.config.pwm_output_startup = {
        'output_0': {
            'mode': 'fixed',
            'max_duty': 0.6,
            'min_duty': 0.0,
            'target_temperature': None,
            'dewpoint_offset': None,
            'ramp_offset': 0.0,
            'duty': 0.6,
        }
    }

    payload = asyncio.run(main.set_pwm_output(_Request({'index': 0, 'mode': 'fixed', 'duty': 0.4, 'apply_at_startup': False})))

    assert main.board.config.pwm_output_startup == {}
    assert payload['pwmOutputs'][0]['apply_at_startup'] is False


def test_write_config_triggers_save(monkeypatch):
    main = _import_main(monkeypatch)
    fake_board = _FakeBoard()
    main.board = fake_board

    payload = asyncio.run(main.write_config(_Request()))
    assert fake_board.config._save_calls == 1
    assert payload['statusLedDuty'] == 1.0


def test_set_pwm_output_ignores_active_input_flag(monkeypatch):
    main = _import_main(monkeypatch)
    main.board = _FakePWMBoard()

    payload = asyncio.run(main.set_pwm_output(_Request({'index': 0, 'mode': 'fixed', 'active': False, 'duty': 0.6})))

    assert main.board.output_pins['output_0'].duty == 0.6
    assert payload['pwmOutputs'][0]['duty'] == 0.6
    assert payload['pwmOutputs'][0]['active'] is True


def test_set_pwm_output_mode_off_forces_output_off(monkeypatch):
    main = _import_main(monkeypatch)
    main.board = _FakePWMBoard()
    main.board.output_pins['output_0'].duty = 0.8

    payload = asyncio.run(main.set_pwm_output(_Request({'index': 0, 'mode': 'off', 'duty': 0.8})))

    assert main.board.output_pins['output_0'].duty == 0.0
    assert payload['pwmOutputs'][0]['duty'] == 0.0
    assert payload['pwmOutputs'][0]['active'] is False

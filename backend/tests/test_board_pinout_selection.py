import os
import asyncio
from types import SimpleNamespace
from typing import Any, cast

import board as board_module


def _new_board():
    return cast(Any, board_module.Board.__new__(board_module.Board))


def _call_private(board: Any, method_name: str, *args: Any):
    return getattr(board, method_name)(*args)


def _set_private(board: Any, attr_name: str, value: Any):
    setattr(board, attr_name, value)


def test_is_pinout_file_name_filters_expected_patterns():
    board = _new_board()

    is_name = getattr(board, '_Board__is_pinout_file_name')
    assert is_name('pinout.json') is True
    assert is_name('pinout_esp32_c3.json') is True
    assert is_name('gpio_pinout_rpi4.json') is False
    assert is_name('pinout_esp32_c3.txt') is False


def test_is_pinout_compatible_uses_hardware_and_variant_profile():
    board = _new_board()
    _set_private(board, '_Board__runtime_hardware_profile', lambda: ('esp32', 'c3'))

    assert _call_private(board, '_Board__is_pinout_compatible', {'hardware': 'esp32', 'variant': 'c3'}) is True
    assert _call_private(board, '_Board__is_pinout_compatible', {'hardware': 'esp32', 'variant': 'any'}) is True
    assert _call_private(board, '_Board__is_pinout_compatible', {'hardware': 'esp32', 'variant': 's2'}) is False
    assert _call_private(board, '_Board__is_pinout_compatible', {'hardware': 'cpython', 'variant': 'c3'}) is False
    assert _call_private(board, '_Board__is_pinout_compatible', {}) is True


def test_resolve_pinout_prefers_configured_file(monkeypatch):
    board = _new_board()
    board.config = SimpleNamespace(pinout_file='pinout_esp32_c3.json')

    monkeypatch.setattr(board_module, 'pinout_config_path', '/defaults/pinout.json')
    monkeypatch.setenv('PINOUT_CONFIG_PATH', '/ignored/from/env.json')

    _set_private(board, '_Board__scan_pinout_files', lambda: [
        {'file': 'pinout.json', 'path': '/defaults/pinout.json', 'metadata': {'hardware': 'cpython'}},
        {'file': 'pinout_esp32_c3.json', 'path': '/custom/pinout_esp32_c3.json', 'metadata': {'hardware': 'cpython'}},
    ])
    _set_private(board, '_Board__is_pinout_compatible', lambda _metadata: True)

    selected = _call_private(board, '_Board__resolve_pinout_config_path')
    assert selected == '/custom/pinout_esp32_c3.json'


def test_resolve_pinout_falls_back_to_first_compatible_when_no_preferred(monkeypatch):
    board = _new_board()
    board.config = SimpleNamespace(pinout_file='')

    missing_default = '/does/not/exist/pinout.json'
    monkeypatch.setattr(board_module, 'pinout_config_path', missing_default)
    monkeypatch.delenv('PINOUT_CONFIG_PATH', raising=False)

    _set_private(board, '_Board__scan_pinout_files', lambda: [
        {'file': 'pinout_a.json', 'path': '/available/pinout_a.json', 'metadata': {'hardware': 'cpython'}},
        {'file': 'pinout_b.json', 'path': '/available/pinout_b.json', 'metadata': {'hardware': 'cpython'}},
    ])
    _set_private(board, '_Board__is_pinout_compatible', lambda _metadata: True)

    selected = _call_private(board, '_Board__resolve_pinout_config_path')
    assert selected == '/available/pinout_a.json'


def test_resolve_pinout_raises_when_nothing_available(monkeypatch):
    board = _new_board()
    board.config = SimpleNamespace(pinout_file='')

    monkeypatch.setattr(board_module, 'pinout_config_path', '/missing/default.json')
    monkeypatch.delenv('PINOUT_CONFIG_PATH', raising=False)

    _set_private(board, '_Board__scan_pinout_files', lambda: [])
    _set_private(board, '_Board__is_pinout_compatible', lambda _metadata: True)

    try:
        _call_private(board, '_Board__resolve_pinout_config_path')
    except RuntimeError as error:
        assert 'No compatible pinout configuration file found' in str(error)
    else:
        raise AssertionError('Expected RuntimeError when no pinout is available')


def test_set_pinout_file_persists_and_marks_restart_requirement():
    board = _new_board()
    save_calls = {'count': 0}

    class _Config:
        pinout_file = ''

        def save(self):
            save_calls['count'] += 1

    board.config = _Config()
    board.pinout_config_file = '/current/pinout.json'
    board.list_available_pinout_files = lambda: [
        {'file': 'pinout_esp32_c3.json', 'path': '/new/pinout_esp32_c3.json'},
    ]

    response = board.set_pinout_file('pinout_esp32_c3.json')
    assert board.config.pinout_file == 'pinout_esp32_c3.json'
    assert save_calls['count'] == 1
    assert response['restartRequired'] is True


def test_set_pinout_file_rejects_paths():
    board = _new_board()
    board.config = SimpleNamespace(pinout_file='', save=lambda: None)
    board.pinout_config_file = '/current/pinout.json'
    board.list_available_pinout_files = lambda: []

    try:
        board.set_pinout_file('../pinout_esp32_c3.json')
    except ValueError as error:
        assert 'file name, not a path' in str(error)
    else:
        raise AssertionError('Expected ValueError for path-like pinout file names')


def test_collect_output_configs_accepts_nested_status_led_output_object():
    board = _new_board()
    board.pinout_config = {
        'pinout': {
            'status_led': {'type': 'pwm', 'pin': 4},
        },
    }

    assert _call_private(board, '_Board__collect_output_configs') == [
        ('status_led', 'status_led', {'type': 'pwm', 'pin': 4}),
    ]


def test_collect_output_configs_preserves_nested_heater_thermistor_pin():
    board = _new_board()
    board.pinout_config = {
        'pinout': {
            'pwm_outputs': [
                {'name': 'PWM0', 'pin': 41, 'type': 'Heater', 'thermistor_pin': 2},
            ]
        },
    }

    assert _call_private(board, '_Board__collect_output_configs') == [
        ('PWM0', 'heater', {'type': 'pwm', 'pin': 41, 'thermistor_pin': 2}),
    ]


def test_resolve_temperature_pin_name_uses_heater_temp_and_thermistor_fields():
    board = _new_board()

    assert _call_private(board, '_Board__resolve_temperature_pin_name', 'heater', {'temp': 'ANALOG_IN0'}) == 'ANALOG_IN0'
    assert _call_private(board, '_Board__resolve_temperature_pin_name', 'heater', {'temp': {'type': 'thermistor', 'pin': 'ANALOG_IN0'}}) == 'ANALOG_IN0'
    assert _call_private(board, '_Board__resolve_temperature_pin_name', 'heater', {'thermistor_pin': 3}) == '3'
    assert _call_private(board, '_Board__resolve_temperature_pin_name', 'heater', {'thermistor_pin': -1}) is None
    assert _call_private(board, '_Board__resolve_temperature_pin_name', 'heater', {'temp': {'type': 'other', 'pin': 'ANALOG_IN0'}}) is None
    assert _call_private(board, '_Board__resolve_temperature_pin_name', 'output', {'temp': 'ANALOG_IN0'}) is None


def test_resolve_thermistor_model_merges_global_and_local_overrides():
    board = _new_board()
    board.pinout_config = {
        'thermistor': {'beta': 3435, 'vcc': 5.0},
    }

    model = _call_private(board, '_Board__resolve_thermistor_model', {'thermistor': {'series_resistor': 4700}})
    assert model['beta'] == 3435.0
    assert model['vcc'] == 5.0
    assert model['series_resistor'] == 4700.0
    assert model['r0'] == 10000.0
    assert model['t0_c'] == 25.0


def test_resolve_thermistor_model_accepts_temp_object_overrides():
    board = _new_board()
    board.pinout_config = {}

    model = _call_private(board, '_Board__resolve_thermistor_model', {
        'temp': {
            'type': 'thermistor',
            'pin': 'ANALOG_IN0',
            'beta': 3435,
            'series_resistor': 4700,
        }
    })
    assert model['type'] == 'thermistor'
    assert model['beta'] == 3435.0
    assert model['series_resistor'] == 4700.0
    assert model['r0'] == 10000.0


def test_resolve_thermistor_model_accepts_nested_pinout_defaults():
    board = _new_board()
    board.pinout_config = {
        'pinout': {
            'thermistor': {'beta': 3380, 'vcc': 5.0, 'wiring': 'ntc_to_vcc'},
        },
    }

    model = _call_private(board, '_Board__resolve_thermistor_model', {})
    assert model['beta'] == 3380.0
    assert model['vcc'] == 5.0
    assert model['wiring'] == 'ntc_to_vcc'


def test_voltage_to_temperature_c_matches_nominal_10k_3950_divider():
    board = _new_board()
    model = {
        'beta': 3950.0,
        'r0': 10000.0,
        't0_c': 25.0,
        'series_resistor': 10000.0,
        'vcc': 3.3,
        'wiring': 'ntc_to_gnd',
    }

    temperature = _call_private(board, '_Board__voltage_to_temperature_c', 1.65, model)
    assert temperature is not None
    assert abs(temperature - 25.0) < 0.2


def test_collect_output_configs_accepts_nested_fan_output_object():
    board = _new_board()
    board.pinout_config = {
        'pinout': {
            'fan': {'type': 'pwm', 'pin': 3},
        },
    }
    board.fan_pin_id = None

    assert _call_private(board, '_Board__collect_output_configs') == [
        ('fan', 'fan', {'type': 'pwm', 'pin': 3}),
    ]
    assert board.fan_pin_id == 'fan'


def test_collect_output_configs_accepts_legacy_nested_fan_pwm_output():
    board = _new_board()
    board.pinout_config = {
        'pinout': {
            'fan_pwm': 3,
        },
    }
    board.fan_pin_id = None

    assert _call_private(board, '_Board__collect_output_configs') == [
        ('fan', 'fan', {'type': 'pwm', 'pin': 3}),
    ]
    assert board.fan_pin_id == 'fan'


def test_restore_pwm_outputs_at_startup_applies_saved_profile():
    board = _new_board()

    class _Pin:
        def __init__(self):
            self.is_pwm = True
            self._duty = 0.0

        @property
        def duty(self):
            return self._duty

        @duty.setter
        def duty(self, value):
            self._duty = value

    board.output_pins = {'heater_0': _Pin(), 'output_0': _Pin()}
    board.config = SimpleNamespace(pwm_output_startup={
        'heater_0': {
            'mode': 'target_temperature',
            'max_duty': 0.55,
            'min_duty': 0.2,
            'target_temperature': 21.0,
            'dewpoint_offset': None,
            'ramp_offset': 1.5,
            'duty': 0.55,
        },
        'output_0': {
            'mode': 'off',
            'max_duty': 0.8,
            'min_duty': 0.0,
            'target_temperature': None,
            'dewpoint_offset': None,
            'ramp_offset': 0.0,
            'duty': 0.8,
        },
        'missing': {
            'mode': 'fixed',
            'max_duty': 0.9,
        },
    })

    board.restore_pwm_outputs_at_startup()

    assert board.output_pins['heater_0'].duty == 0.55
    assert board.output_pins['output_0'].duty == 0.0


def test_start_applies_fan_duty_before_starting_status_led():
    board = _new_board()
    call_order = []

    board.apply_fan_duty = lambda: call_order.append('fan')
    board.config = SimpleNamespace(pwm_output_startup={})

    class _StatusLed:
        async def start(self):
            call_order.append('status')

    board.status_led = _StatusLed()

    asyncio.run(board.start())
    assert call_order == ['fan', 'status']


def test_gpio_initialization_error_identifies_the_application_pin(monkeypatch):
    board = _new_board()

    class _BusyPin:
        def __init__(self, _pin_name):
            raise RuntimeError('GPIO busy')

    monkeypatch.setattr(board_module.gpio, 'PWMOutputPin', _BusyPin)
    monkeypatch.setattr(board_module.gpio, 'ButtonPin', _BusyPin)

    try:
        _call_private(board, '_Board__load_output', 'heater_0', 'heater', {'type': 'pwm', 'pin': 'PWM0'})
    except RuntimeError as error:
        assert str(error) == "Failed to initialize heater pin 'heater_0' configured as 'PWM0': GPIO busy"
        assert isinstance(error.__cause__, RuntimeError)
        assert str(error.__cause__) == 'GPIO busy'
    else:
        raise AssertionError('Expected contextual GPIO initialization error')

    try:
        _call_private(board, '_Board__load_button', 'button_0', 'BTN0')
    except RuntimeError as error:
        assert str(error) == "Failed to initialize button pin 'button_0' configured as 'BTN0': GPIO busy"
        assert isinstance(error.__cause__, RuntimeError)
        assert str(error.__cause__) == 'GPIO busy'
    else:
        raise AssertionError('Expected contextual GPIO initialization error')


def test_pin_initialization_logs_reused_configured_pin(caplog):
    board = _new_board()

    with caplog.at_level('DEBUG', logger='board'):
        _call_private(board, '_Board__log_pin_initialization', {}, 'heater_0', 'heater', 'pwm', 'PWM5')
        _call_private(board, '_Board__log_pin_initialization', {'PWM5': 'heater_0'}, 'output_2', 'output', 'pwm', 'PWM5')

    assert "Initializing pin 'heater_0' (heater, pwm) configured as 'PWM5'" in caplog.messages
    assert "Pin 'output_2' (output, pwm) reuses configured pin 'PWM5' already used by 'heater_0'" in caplog.messages

import os
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

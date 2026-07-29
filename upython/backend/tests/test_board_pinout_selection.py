import os
from types import SimpleNamespace

import pytest

import board as board_module


def _new_board():
    return board_module.Board.__new__(board_module.Board)


def test_is_pinout_file_name_filters_expected_patterns():
    board = _new_board()

    is_name = board._Board__is_pinout_file_name
    assert is_name('pinout.json') is True
    assert is_name('pinout_esp32_c3.json') is True
    assert is_name('gpio_pinout_rpi4.json') is False
    assert is_name('pinout_esp32_c3.txt') is False


def test_is_pinout_compatible_uses_hardware_and_variant_profile():
    board = _new_board()
    board._Board__runtime_hardware_profile = lambda: ('esp32', 'c3')

    assert board._Board__is_pinout_compatible({'hardware': 'esp32', 'variant': 'c3'}) is True
    assert board._Board__is_pinout_compatible({'hardware': 'esp32', 'variant': 'any'}) is True
    assert board._Board__is_pinout_compatible({'hardware': 'esp32', 'variant': 's2'}) is False
    assert board._Board__is_pinout_compatible({'hardware': 'cpython', 'variant': 'c3'}) is False
    assert board._Board__is_pinout_compatible({}) is True


def test_resolve_pinout_prefers_configured_file(monkeypatch):
    board = _new_board()
    board.config = SimpleNamespace(pinout_file='pinout_esp32_c3.json')

    monkeypatch.setattr(board_module, 'pinout_config_path', '/defaults/pinout.json')
    monkeypatch.setenv('PINOUT_CONFIG_PATH', '/ignored/from/env.json')

    board._Board__scan_pinout_files = lambda: [
        {'file': 'pinout.json', 'path': '/defaults/pinout.json', 'metadata': {'hardware': 'cpython'}},
        {'file': 'pinout_esp32_c3.json', 'path': '/custom/pinout_esp32_c3.json', 'metadata': {'hardware': 'cpython'}},
    ]
    board._Board__is_pinout_compatible = lambda _metadata: True

    selected = board._Board__resolve_pinout_config_path()
    assert selected == '/custom/pinout_esp32_c3.json'


def test_resolve_pinout_falls_back_to_first_compatible_when_no_preferred(monkeypatch):
    board = _new_board()
    board.config = SimpleNamespace(pinout_file='')

    missing_default = '/does/not/exist/pinout.json'
    monkeypatch.setattr(board_module, 'pinout_config_path', missing_default)
    monkeypatch.delenv('PINOUT_CONFIG_PATH', raising=False)

    board._Board__scan_pinout_files = lambda: [
        {'file': 'pinout_a.json', 'path': '/available/pinout_a.json', 'metadata': {'hardware': 'cpython'}},
        {'file': 'pinout_b.json', 'path': '/available/pinout_b.json', 'metadata': {'hardware': 'cpython'}},
    ]
    board._Board__is_pinout_compatible = lambda _metadata: True

    selected = board._Board__resolve_pinout_config_path()
    assert selected == '/available/pinout_a.json'


def test_resolve_pinout_raises_when_nothing_available(monkeypatch):
    board = _new_board()
    board.config = SimpleNamespace(pinout_file='')

    monkeypatch.setattr(board_module, 'pinout_config_path', '/missing/default.json')
    monkeypatch.delenv('PINOUT_CONFIG_PATH', raising=False)

    board._Board__scan_pinout_files = lambda: []
    board._Board__is_pinout_compatible = lambda _metadata: True

    with pytest.raises(RuntimeError, match='No compatible pinout configuration file found'):
        board._Board__resolve_pinout_config_path()


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

    with pytest.raises(ValueError, match='file name, not a path'):
        board.set_pinout_file('../pinout_esp32_c3.json')

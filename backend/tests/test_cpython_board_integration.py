import importlib
import json

import pytest

import board
import board_compat
from boards.cpython.json_config_storage import JsonConfigStorage


class _Lgpio:
    BOTH_EDGES = 3

    def __init__(self):
        self.writes = []
        self.pwm = []
        self.callbacks = []

    def gpiochip_open(self, chip):
        return chip

    def gpio_claim_output(self, *_args):
        return None

    def gpio_write(self, handle, line, value):
        self.writes.append((handle, line, value))

    def tx_pwm(self, handle, line, frequency, duty):
        self.pwm.append((handle, line, frequency, duty))

    def gpio_get_mode(self, *_args):
        return 0

    def gpio_claim_alert(self, *_args):
        return None

    def gpio_read(self, *_args):
        return 1

    def callback(self, _handle, line, _edges, callback):
        self.callbacks.append((line, callback))
        return type('Callback', (), {'cancel': lambda self: None})()


@pytest.fixture
def cpython_board(monkeypatch, tmp_path):
    pinout_path = tmp_path / 'pinout.json'
    pinout_path.write_text(
        json.dumps(
            {
                'status_led': {'type': 'pwm', 'pin': 'LED'},
                'outputs': [
                    {'type': 'digital', 'pin': 'POWER'},
                    {'type': 'pwm', 'pin': 'PWM'},
                ],
                'buttons': ['BUTTON'],
            }
        )
    )
    gpio_config_path = tmp_path / 'gpio.json'
    gpio_config_path.write_text(
        json.dumps(
            {
                'pinout': {
                    'LED': 'GPIO0_A1',
                    'POWER': 'GPIO0_A2',
                    'PWM': 'GPIO0_A3',
                    'BUTTON': 'GPIO0_A4',
                }
            }
        )
    )
    original_config_path = JsonConfigStorage.CONFIG_FILE_PATH

    try:
        with monkeypatch.context() as environment:
            environment.delenv('SIMULATOR_GPIO', raising=False)
            environment.setenv('PINOUT_CONFIG_PATH', str(pinout_path))
            environment.setenv('GPIO_CONFIG_PATH', str(gpio_config_path))
            environment.setattr(JsonConfigStorage, 'CONFIG_FILE_PATH', str(tmp_path / 'config.json'))

            importlib.reload(board_compat)
            board_module = importlib.reload(board)
            fake_lgpio = _Lgpio()
            environment.setattr(board_compat.gpio, 'lgpio', fake_lgpio)
            environment.setattr(board_compat.gpio, '_pin_config', None)
            yield board_module.Board(), fake_lgpio
    finally:
        JsonConfigStorage.CONFIG_FILE_PATH = original_config_path
        importlib.reload(board_compat)
        importlib.reload(board)


def test_cpython_board_composes_real_gpio_adapters_and_publishes_events(cpython_board):
    board_instance, fake_lgpio = cpython_board
    events = []
    board_instance.on_pin_update(events.append)

    board_instance.output_pins['output_0'].on = True
    board_instance.output_pins['output_1'].duty = 0.6

    assert (0, 2, 1) in fake_lgpio.writes
    assert (0, 3, 1000, 60.0) in fake_lgpio.pwm
    assert events[-1]['output_1'] == {
        'duty': 0.6,
        'on': True,
    }

    button_line, button_callback = fake_lgpio.callbacks[-1]
    button_callback(0, button_line, 0, 0)

    assert events[-1]['button_0'] == {
        'on': True,
    }

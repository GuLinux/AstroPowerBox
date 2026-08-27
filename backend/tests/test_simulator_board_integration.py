import importlib
import json
from pathlib import Path

import pytest

import board
import board_compat
from boards.cpython.json_config_storage import JsonConfigStorage


PINOUT_PATH = Path(__file__).resolve().parents[1] / 'config_files' / 'pinout.json'


@pytest.fixture
def simulator_board(monkeypatch, tmp_path):
    original_config_path = JsonConfigStorage.CONFIG_FILE_PATH

    try:
        with monkeypatch.context() as environment:
            environment.setenv('SIMULATOR_GPIO', '1')
            environment.setenv('PINOUT_CONFIG_PATH', str(PINOUT_PATH))
            environment.setattr(JsonConfigStorage, 'CONFIG_FILE_PATH', str(tmp_path / 'config.json'))

            importlib.reload(board_compat)
            board_module = importlib.reload(board)
            import boards.simulator.gpio as simulator_gpio

            def init_gpio(self, pin_name):
                self.pin_name = pin_name
                self.pin_file = str(tmp_path / pin_name)

            environment.setattr(simulator_gpio.GPIO, '__init__', init_gpio)
            yield board_module.Board(), board_module, tmp_path
    finally:
        JsonConfigStorage.CONFIG_FILE_PATH = original_config_path
        importlib.reload(board_compat)
        importlib.reload(board)


def test_simulator_board_publishes_pin_events_for_output_and_button(simulator_board):
    board_instance, _, tmp_path = simulator_board
    events = []
    board_instance.on_pin_update(events.append)

    heater = board_instance.output_pins['heater_0']
    heater.duty = 0.4

    assert (tmp_path / 'PWM0').read_text() == '0.4'
    assert events[-1]['heater_0'] == {
        'duty': 0.4,
        'on': True,
    }

    board_instance.button_pins['button_0']._trigger_callback(True)
    snapshot = board_instance.pin_status_snapshot()
    button = next(pin for pin in snapshot['pins'] if pin['id'] == 'button_0')

    assert button == {
        'id': 'button_0',
        'role': 'button',
        'kind': 'button',
        'on': True,
    }
    assert events[-1]['button_0'] == {'on': True}


def test_simulator_board_persists_configuration_and_reloads_it(simulator_board):
    board_instance, board_module, tmp_path = simulator_board
    board_instance.config.status_led_duty = 0.25
    board_instance.config.pinout_file = 'pinout.json'
    board_instance.config.save()

    saved = json.loads((tmp_path / 'config.json').read_text())
    reloaded = board_module.Board()

    assert saved['stLedDuty'] == 0.25
    assert saved['pinoutFile'] == 'pinout.json'
    assert reloaded.config.status_led_duty == 0.25
    assert reloaded.get_pinout_selection()['selectedFile'] == 'pinout.json'

import boards.simulator.gpio as simulator_gpio


def _use_temporary_gpio_files(monkeypatch, tmp_path):
    def init(self, pin_name):
        self.pin_name = pin_name
        self.pin_file = str(tmp_path / pin_name)

    monkeypatch.setattr(simulator_gpio.GPIO, '__init__', init)


def test_digital_output_updates_file_and_notifies_only_on_changes(monkeypatch, tmp_path):
    _use_temporary_gpio_files(monkeypatch, tmp_path)
    pin = simulator_gpio.DigitalOutputPin('POWER')
    changes = []
    pin.on_level_changed(changes.append)

    pin.duty = 1.0
    pin.duty = 0.2
    pin.duty = 0.0

    assert pin.is_pwm is False
    assert pin.on is False
    assert pin.duty == 0.0
    assert changes == [True, False]
    assert (tmp_path / 'POWER').read_text() == 'False'


def test_pwm_output_clamps_duty_and_notifies_only_on_changes(monkeypatch, tmp_path):
    _use_temporary_gpio_files(monkeypatch, tmp_path)
    pin = simulator_gpio.PWMOutputPin('PWM1')
    changes = []
    pin.on_duty_changed(changes.append)

    pin.duty = 1.5
    pin.duty = 1.0
    pin.duty = -0.5

    assert pin.is_pwm is True
    assert pin.on is False
    assert pin.duty == 0.0
    assert changes == [1.0, 0.0]
    assert (tmp_path / 'PWM1').read_text() == '0.0'


def test_button_and_analog_inputs_expose_simulated_state(monkeypatch, tmp_path):
    _use_temporary_gpio_files(monkeypatch, tmp_path)
    button = simulator_gpio.ButtonPin('BUTTON')
    changes = []
    button.on_level_changed(changes.append)
    button._trigger_callback(True)
    button._trigger_callback(False)

    analog = simulator_gpio.AnalogInputPin('VOLTAGE')
    analog.set_simulated_value(5.0)

    assert button.value is False
    assert changes == [True, False]
    assert analog.value == 4.096

import pytest

import boards.cpython.gpio as gpio


class _Callback:
    def __init__(self):
        self.cancelled = False

    def cancel(self):
        self.cancelled = True


class _Lgpio:
    BOTH_EDGES = 3

    def __init__(self):
        self.calls = []
        self.reads = {}
        self.callbacks = []

    def gpiochip_open(self, chip):
        self.calls.append(('open', chip))
        return chip

    def gpio_claim_output(self, handle, line):
        self.calls.append(('claim_output', handle, line))

    def gpio_write(self, handle, line, value):
        self.calls.append(('write', handle, line, value))

    def tx_pwm(self, handle, line, frequency, duty):
        self.calls.append(('pwm', handle, line, frequency, duty))

    def gpio_get_mode(self, handle, line):
        self.calls.append(('get_mode', handle, line))
        return 0

    def gpio_claim_alert(self, handle, line, edges, mode):
        self.calls.append(('claim_alert', handle, line, edges, mode))

    def gpio_read(self, handle, line):
        return self.reads.get((handle, line), 1)

    def callback(self, handle, line, edges, callback):
        handle = _Callback()
        self.callbacks.append((line, callback, handle))
        return handle


class _PinConfig:
    def get_gpio_pin(self, pin_name):
        return {
            'DIGITAL': 'GPIO1_B3',
            'PWM': 'GPIO2_C4',
            'BUTTON': 'GPIO0_A5',
        }[pin_name]

    def get_adc_config(self, pin_name):
        assert pin_name == 'ANALOG'
        return {'i2c_bus': 1, 'i2c_addr': 72, 'channel': 2}


class _AdsDevice:
    PGA_4_096V = 'gain'

    def __init__(self, bus, address):
        self.bus = bus
        self.address = address
        self.gains = []

    def setGain(self, gain):
        self.gains.append(gain)

    def toVoltage(self):
        return 0.01

    def readADC(self, channel):
        assert channel == 2
        return 123


class _AdsModule:
    ADS1115 = _AdsDevice


@pytest.fixture
def fake_gpio(monkeypatch):
    fake_lgpio = _Lgpio()
    monkeypatch.setattr(gpio, 'lgpio', fake_lgpio)
    monkeypatch.setattr(gpio, '_pin_config', _PinConfig())
    monkeypatch.setattr(gpio, '_ads1115_available', True)
    monkeypatch.setattr(gpio, 'ADS1x15', _AdsModule, raising=False)
    gpio.AnalogInputPin._ads_instances = {}
    return fake_lgpio


def test_parse_gpio_pin_validates_and_calculates_line_offsets():
    assert gpio._parse_gpio_pin('gpio2_c4') == (2, 20)

    with pytest.raises(ValueError, match='Invalid GPIO format'):
        gpio._parse_gpio_pin('invalid')


def test_digital_output_writes_changes_and_notifies_callbacks(fake_gpio):
    pin = gpio.DigitalOutputPin('DIGITAL')
    changes = []
    pin.on_level_changed(changes.append)

    pin.duty = 0.5
    pin.duty = 1.0
    pin.on = False

    assert pin.is_pwm is False
    assert pin.duty == 0.0
    assert changes == [True, False]
    assert ('claim_output', 1, 11) in fake_gpio.calls
    assert ('write', 1, 11, 1) in fake_gpio.calls
    assert ('write', 1, 11, 0) in fake_gpio.calls


def test_pwm_output_clamps_duty_and_notifies_callbacks(fake_gpio):
    pin = gpio.PWMOutputPin('PWM', frequency=250)
    changes = []
    pin.on_duty_changed(changes.append)

    pin.duty = 1.5
    pin.duty = 1.0
    pin.on = False

    assert pin.is_pwm is True
    assert pin.duty == 0.0
    assert changes == [1.0, 0.0]
    assert ('pwm', 2, 20, 250, 100.0) in fake_gpio.calls
    assert ('pwm', 2, 20, 250, 0.0) in fake_gpio.calls


def test_button_and_analog_input_use_hardware_adapters(fake_gpio):
    button = gpio.ButtonPin('BUTTON')
    fake_gpio.reads[(0, 5)] = 0
    changes = []
    button.on_level_changed(changes.append)
    line, callback, first_handle = fake_gpio.callbacks[-1]
    callback(0, line, 0, 0)
    button.on_level_changed(changes.append)

    analog = gpio.AnalogInputPin('ANALOG')

    assert button.value is True
    assert changes == [True]
    assert first_handle.cancelled is True
    assert analog.value == 1.23
    assert analog.ads.gains == ['gain']

def _skip(message):
    try:
        import pytest

        pytest.skip(message)
    except Exception:
        from tests_mpy.run_tests import SkipTest

        raise SkipTest(message)


def _require_micropython():
    import sys

    if getattr(sys.implementation, 'name', '') != 'micropython':
        _skip('MicroPython runtime required')


def test_esp32_runtime_is_available():
    _require_micropython()
    from machine import unique_id

    if not unique_id():
        raise AssertionError('ESP32 unique ID is empty')


def test_status_led_pwm_output_on_device():
    _require_micropython()
    import json
    import time

    board_name = None
    try:
        from board_vars import board_name
    except ImportError:
        _skip('board_vars is not deployed')

    if board_name is None:
        _skip('board_vars did not expose board_name')

    from boards.esp32.gpio import PWMOutputPin

    with open('/pinout_{}.json'.format(board_name), 'r') as pinout_file:
        pinout = json.load(pinout_file)

    status_led = pinout['pinout']['status_led']
    if not isinstance(status_led, dict) or status_led.get('type') != 'pwm':
        raise AssertionError(f'Selected board {board_name} must define a PWM status LED for HIL testing, got {status_led}')

    output = PWMOutputPin(str(status_led['pin']))
    changes = []
    output.on_duty_changed(changes.append)
    output.duty = 0.02
    output.duty = 0.0
    for _ in range(10):
        if output.duty == 0.0 and changes == [0.02, 0.0]:
            break    
        time.sleep(0.5)

    if output.duty != 0.0:
        raise AssertionError('Status LED did not return to off')
    if changes != [0.02, 0.0]:
        raise AssertionError('Status LED duty callbacks did not report both changes')


class _MemoryStorage:
    def __init__(self):
        self.data = {}

    def load_str(self, key, maximum_size=1024, default=''):
        return self.data.get(key, default)

    def load_float(self, key, default=0.0):
        return self.data.get(key, default)

    def load_int(self, key, default=0):
        return self.data.get(key, default)

    def load_json(self, key, maximum_size=1024, default=None):
        if default is None:
            default = {}
        return self.data.get(key, default)

    def load_wifi(self):
        from wifi import WiFi

        ap = self.data.get('ap', {'ssid': 'AstroPowerBox', 'psk': 'astropowerbox'})
        stations = self.data.get('stations', [])
        return WiFi.from_json(ap), WiFi.from_json_list(stations)

    def save_str(self, key, value):
        self.data[key] = value

    def save_float(self, key, value):
        self.data[key] = value

    def save_int(self, key, value):
        self.data[key] = value

    def save_json(self, key, value):
        self.data[key] = value

    def save_wifi(self, stations, ap):
        from wifi import WiFi

        self.data['stations'] = WiFi.to_json_list(stations)
        self.data['ap'] = ap.json


def _new_config(stations):
    from config import Config
    from wifi import WiFi

    storage = _MemoryStorage()
    config = Config(storage)
    config.ap = WiFi('AstroPowerBox-Test', 'astropowerbox')
    config.stations = stations
    return config


def test_wifi_connects_to_configured_station_from_env():
    _require_micropython()
    import uasyncio as asyncio
    from boards.esp32.wifi_manager import ESPWiFiManager
    from wifi import WiFi
    from tests_mpy import wifi_test_env

    if not wifi_test_env.STA_SSID:
        raise AssertionError('APB_TEST_WIFI_SSID must be set for ESP32 station connectivity test')

    configured_station = WiFi(wifi_test_env.STA_SSID, wifi_test_env.STA_PSK or None)
    manager = ESPWiFiManager(_new_config([configured_station]))
    state = {
        'connecting': 0,
        'station': None,
        'ap': None,
    }
    manager.on_connecting = lambda: state.__setitem__('connecting', state['connecting'] + 1)
    manager.on_station_connected = lambda ssid: state.__setitem__('station', ssid)
    manager.on_ap_started = lambda ssid: state.__setitem__('ap', ssid)

    asyncio.run(manager.connect_stations(connect_ap_on_failure=True))

    if state['connecting'] != 1:
        raise AssertionError('Wi-Fi manager did not emit a connecting event exactly once')
    if not manager.station_wlan.isconnected():
        raise AssertionError('Station did not connect to the configured SSID')
    if state['station'] != wifi_test_env.STA_SSID:
        raise AssertionError('Wi-Fi manager did not report the connected station SSID')
    if state['ap'] is not None:
        raise AssertionError('AP mode should not start when station connection succeeds')

    try:
        manager.station_wlan.disconnect()
    except Exception:
        pass


def test_wifi_falls_back_to_ap_mode_when_station_is_unavailable():
    _require_micropython()
    import uasyncio as asyncio
    from boards.esp32.wifi_manager import ESPWiFiManager
    from wifi import WiFi
    from tests_mpy import wifi_test_env

    bad_ssid = wifi_test_env.BAD_SSID or 'BadSSID'
    bad_psk = wifi_test_env.BAD_PSK or None

    manager = ESPWiFiManager(_new_config([WiFi(bad_ssid, bad_psk)]))
    scanned_ssids = [network.ssid for network in manager.scan()]
    if bad_ssid in scanned_ssids:
        _skip('Configured APB_TEST_WIFI_BAD_SSID is visible; choose a guaranteed non-existent SSID')

    state = {
        'station': None,
        'ap': None,
    }
    manager.on_station_connected = lambda ssid: state.__setitem__('station', ssid)
    manager.on_ap_started = lambda ssid: state.__setitem__('ap', ssid)

    asyncio.run(manager.connect_stations(connect_ap_on_failure=True))

    if state['station'] is not None:
        raise AssertionError('Station callback should not fire when connection fails')
    if state['ap'] != manager.config.ap.ssid:
        raise AssertionError('Wi-Fi manager did not start AP mode after station failures')
    if not manager.ap_wlan.active():
        raise AssertionError('AP interface is not active after fallback')

    manager.ap_wlan.active(False)

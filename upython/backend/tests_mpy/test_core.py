from wifi import WiFi
from utils import check_required_key


def _assert(condition, message):
    if not condition:
        raise AssertionError(message)


def test_wifi_requires_ssid():
    try:
        WiFi.from_json({'psk': 'secret'})
    except KeyError:
        return
    raise AssertionError('WiFi.from_json should raise KeyError when ssid is missing')


def test_wifi_roundtrip():
    source = [WiFi('A', 'a123'), WiFi('B', '')]
    payload = WiFi.to_json_list(source)
    loaded = WiFi.from_json_list(payload)
    _assert(len(loaded) == 2, 'Expected two WiFi entries')
    _assert(loaded[0].ssid == 'A', 'Unexpected SSID for first WiFi')
    _assert(loaded[1].ssid == 'B', 'Unexpected SSID for second WiFi')


def test_check_required_key_raises():
    try:
        check_required_key('ssid', {})
    except KeyError:
        return
    raise AssertionError('check_required_key should raise when key is missing')

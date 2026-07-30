import pytest

from wifi import WiFi


def test_wifi_from_json_requires_ssid():
    with pytest.raises(KeyError):
        WiFi.from_json({'psk': 'secret'})


def test_wifi_from_json_defaults_psk_to_empty_string():
    wifi = WiFi.from_json({'ssid': 'AP'})
    assert wifi.ssid == 'AP'
    assert wifi.psk == None


def test_wifi_to_and_from_json_list_roundtrip():
    source = [
        WiFi('A', 'a123'),
        WiFi('B', ''),
    ]

    payload = WiFi.to_json_list(source)
    loaded = WiFi.from_json_list(payload)

    assert len(loaded) == 2
    assert loaded[0].ssid == 'A'
    assert loaded[0].psk == 'a123'
    assert loaded[1].ssid == 'B'
    assert loaded[1].psk == ''

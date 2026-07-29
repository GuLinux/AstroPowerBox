import pytest

from utils import check_required_key, check_required_keys


def test_check_required_key_raises_for_missing_key():
    with pytest.raises(KeyError):
        check_required_key('ssid', {})


def test_check_required_keys_accepts_complete_object():
    payload = {'ssid': 'AP', 'psk': 'secret'}
    check_required_keys(['ssid', 'psk'], payload)

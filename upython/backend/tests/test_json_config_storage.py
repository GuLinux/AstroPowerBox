import json

from boards.cpython.json_config_storage import JsonConfigStorage


def _new_storage(tmp_path):
    config_path = tmp_path / 'config.json'
    JsonConfigStorage.CONFIG_FILE_PATH = str(config_path)
    return JsonConfigStorage(), config_path


def test_load_defaults_when_file_is_missing(tmp_path):
    storage, _ = _new_storage(tmp_path)

    assert storage.load_str('missing', default='x') == 'x'
    assert storage.load_int('missing', default=7) == 7
    assert storage.load_float('missing', default=1.5) == 1.5
    assert storage.load_json('missing', default={'k': 'v'}) == {'k': 'v'}


def test_save_and_load_roundtrip(tmp_path):
    storage, config_path = _new_storage(tmp_path)

    storage.save_str('ssid', 'AstroPowerBox')
    storage.save_int('retries', 3)
    storage.save_float('duty', 0.75)
    storage.save_json('pinout', {'file': 'pinout_esp32_c3.json'})

    assert config_path.exists()
    loaded = json.loads(config_path.read_text())
    assert loaded['ssid'] == 'AstroPowerBox'
    assert loaded['retries'] == 3
    assert loaded['duty'] == 0.75
    assert loaded['pinout']['file'] == 'pinout_esp32_c3.json'

    reloaded = JsonConfigStorage()
    assert reloaded.load_str('ssid') == 'AstroPowerBox'
    assert reloaded.load_int('retries') == 3
    assert reloaded.load_float('duty') == 0.75
    assert reloaded.load_json('pinout') == {'file': 'pinout_esp32_c3.json'}


def test_invalid_json_file_is_treated_as_empty_config(tmp_path):
    storage, config_path = _new_storage(tmp_path)
    config_path.write_text('{invalid json')

    assert storage.load_str('ssid', default='fallback') == 'fallback'

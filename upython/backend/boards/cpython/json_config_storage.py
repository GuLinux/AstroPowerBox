import json
import os
from protocols.config_storage import ConfigStorage

class JsonConfigStorage(ConfigStorage):
    CONFIG_FILE_PATH = os.environ.get('CONFIG_FILE', 'config_files/config.json')
    def __init__(self):
        self.config_file = JsonConfigStorage.CONFIG_FILE_PATH

    def load_str(self, key: str, maximum_size: int=1024, default: str='') -> str | None:
        return self.__load_json_config().get(key, default)

    def load_int(self, key: str, default: int=0) -> int | None:
        return self.__load_json_config().get(key, default)

    def load_float(self, key: str, default: float=0.0) -> float | None:
        return self.__load_json_config().get(key, default)

    def load_json(self, key: str, maximum_size: int=1024, default: dict | list={}) -> dict | list | None:
        return self.__load_json_config().get(key, default)
 
    def save_str(self, key: str, value: str):
        config = self.__load_json_config()
        config[key] = value
        self.__save_json_config(config)

    def save_int(self, key: str, value: int):
        config = self.__load_json_config()
        config[key] = value
        self.__save_json_config(config)

    def save_float(self, key: str, value: float):
        config = self.__load_json_config()
        config[key] = value
        self.__save_json_config(config)
       
    def save_json(self, key: str, value: dict | list):
        config = self.__load_json_config()
        config[key] = value
        self.__save_json_config(config)

    def __load_json_config(self) -> dict:
        if not os.path.exists(self.config_file):
            return {}
        with open(self.config_file, 'r') as f:
            try:
                return json.load(f)
            except json.JSONDecodeError:
                print('Failed to decode JSON config file, using empty config')
                return {}

    def __save_json_config(self, config: dict):
        with open(self.config_file, 'w') as f:
            json.dump(config, f, indent=4)

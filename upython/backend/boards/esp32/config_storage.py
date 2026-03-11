import esp32
import struct
import json
from protocols.config_storage import ConfigStorage

class ESPConfigStorage(ConfigStorage):
    def __init__(self):
        self.nvs = esp32.NVS("apb_config")
        self.version = self.load_int('version') or 0

    def load_str(self, key: str, maximum_size: int=1024, default: str='') -> str | None:
        blob = self._load_blob(key, buffer_size=maximum_size)
        return blob.decode('utf-8') if blob is not None else default

    def load_int(self, key: str, default: int=0) -> int | None:
        blob = self._load_blob(key, buffer_size=4)
        return int.from_bytes(blob, 'big') if blob is not None else default

    def load_float(self, key: str, default: float=0.0) -> float | None:
        blob = self._load_blob(key, 4)
        return struct.unpack('f', blob)[0] if blob is not None else default

    def load_json(self, key: str, maximum_size: int=1024, default: dict | list={}) -> dict | list | None:
        blob = self._load_blob(key, buffer_size=maximum_size)
        return json.loads(blob.decode('utf-8')) if blob is not None else default
 
    def save_str(self, key: str, value: str):
        self._save_blob(key, value.encode('utf-8'))

    def save_int(self, key: str, value: int):
        self._save_blob(key, value.to_bytes(4, 'big'))

    def save_float(self, key: str, value: float):
        self._save_blob(key, struct.pack('f', value))
       
    def save_json(self, key: str, value: dict | list):
        self._save_blob(key, json.dumps(value).encode('utf-8'))


    def _save_blob(self, key: str, value: bytes):
        self.nvs.set_blob(key, value)
        self.nvs.commit()

    def _load_blob(self, key: str, buffer_size: int, default_value: bytes | None = None) -> bytes | None:
        buffer = bytearray(buffer_size)
        try:
            read = self.nvs.get_blob(key, buffer)
            return buffer
        except OSError:
            return default_value

from typing import Protocol

class ConfigStorage(Protocol):

    def load_str(self, key: str, maximum_size: int=1024, default: str='') -> str | None:
        raise NotImplementedError()

    def load_int(self, key: str, default: int=0) -> int | None:
        raise NotImplementedError()

    def load_float(self, key: str, default: float=0.0) -> float | None:
        raise NotImplementedError()

    def load_json(self, key: str, maximum_size: int=1024, default: dict | list={}) -> dict | list | None:
        raise NotImplementedError()
 
    def save_str(self, key: str, value: str) -> None:
        raise NotImplementedError()

    def save_int(self, key: str, value: int) -> None:
        raise NotImplementedError()

    def save_float(self, key: str, value: float) -> None:
        raise NotImplementedError()
       
    def save_json(self, key: str, value: dict | list) -> None:
        raise NotImplementedError()
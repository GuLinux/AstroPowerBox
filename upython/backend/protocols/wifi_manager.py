from protocols.config import Config
from typing import Protocol
from typing import Callable


class WiFiManager(Protocol):
    _on_ap_started: Callable[[str], None]
    _on_station_connected: Callable[[str], None]
    _on_connecting: Callable[[], None]

    def __init__(self, config: Config):
        self._on_ap_started = lambda ssid: None
        self._on_station_connected = lambda ssid: None
        self._on_connecting = lambda: None

    async def connect_stations(self, connect_ap_on_failure: bool = True):
        raise NotImplementedError()

    async def start_ap(self):
        raise NotImplementedError()

    def set_hostname(self):
        raise NotImplementedError()

    @property
    def on_station_connected(self) -> Callable[[str], None]:
        return self._on_station_connected

    @on_station_connected.setter
    def on_station_connected(self, callback: Callable[[str], None]) -> None:
        self._on_station_connected = callback

    @property
    def on_ap_started(self) -> Callable[[str], None]:
        return self._on_ap_started    
    
    @on_ap_started.setter
    def on_ap_started(self, callback: Callable[[str], None]) -> None:
        self._on_ap_started = callback

    @property
    def on_connecting(self) -> Callable[[], None]:
        return self._on_connecting
    
    @on_connecting.setter
    def on_connecting(self, callback: Callable[[], None]) -> None:
        self._on_connecting = callback
    
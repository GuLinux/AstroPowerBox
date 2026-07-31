from protocols.typing_compat import Protocol
from wifi import WiFi

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

    def save_wifi(self, stations: list[WiFi], ap: WiFi) -> None:
        self.save_json('stations', WiFi.to_json_list(stations))
        self.save_json('ap', ap.json)

    def load_wifi(self) -> tuple[WiFi, list[WiFi]]:
        stations = self.__read_wifi_stations()
        ap = self.__read_wifi_ap()
        return ap, stations

    def __read_wifi_stations(self, buffer_size=2000) -> list[WiFi]:
        stations_json = self.load_json('stations', maximum_size=buffer_size, default=[])
        if type(stations_json) is not list:
            raise ValueError('Stations config must be a JSON array')
        return WiFi.from_json_list(stations_json)

    def __read_wifi_ap(self, buffer_size=512) -> WiFi:
        ap_json = self.load_json('ap', maximum_size=buffer_size, default=WiFi(ssid='AstroPowerBox', psk='astropowerbox').json)
        if type(ap_json) is not dict:
            raise ValueError('AP config must be a JSON object')
        return WiFi.from_json(ap_json)

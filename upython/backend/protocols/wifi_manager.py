from protocols.config import Config


class WiFiManager:
    def __init__(self, config: Config):
        pass

    def scan(self) -> list[str]:
        raise NotImplementedError()

    async def connect_stations(self, connect_ap_on_failure: bool = True):
        raise NotImplementedError()

    async def start_ap(self):
        raise NotImplementedError()

    def set_hostname(self):
        raise NotImplementedError()



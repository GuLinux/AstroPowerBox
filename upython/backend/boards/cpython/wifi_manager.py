import protocols.wifi_manager
from protocols.config import Config
from wifi import WiFi
from boards.cpython.net.netplan import NetPlanConfig
from boards.cpython.net.networkmanager import NetworkManager


class WiFiManager(protocols.wifi_manager.WiFiManager):
    def __init__(self, config: Config):
        super().__init__(config)
        self.config = config
        self.netplan_config = NetPlanConfig()
        self.network_manager = NetworkManager()

    async def connect_stations(self, connect_ap_on_failure: bool = True):
        device = await self.network_manager.get_wifi_device()
        if not device:
            print('No WiFi device found, skipping station connection')
            if connect_ap_on_failure:
                await self.start_ap()
            return

        self.on_connecting()
        await self.write_wifi_config(device, self.config.ap, self.config.stations)

        print('Connecting to WiFi stations...')
        for station in self.config.stations:
            connection_name = self.netplan_config.station_connection_name(station.ssid)
            print(f'Attempting to connect to station: {station.ssid}')
            if await self.network_manager.connect_station(connection_name, device):
                print(f'Connected to station: {station.ssid}')
                self.on_station_connected(station.ssid)
                return
            print(f'Failed to connect to station: {station.ssid}')

        print('Failed to connect to any station')
        if connect_ap_on_failure:
            await self.start_ap()

    async def start_ap(self):
        device = await self.network_manager.get_wifi_device()
        if not device:
            raise RuntimeError('No WiFi device found, cannot start AP mode')

        await self.write_wifi_config(device, self.config.ap, self.config.stations)
        connection_name = self.netplan_config.ap_connection_name()
        print(f'Starting AP with SSID: {self.config.ap.ssid}')
        if not await self.network_manager.start_ap(connection_name, device):
            raise RuntimeError(f'Failed to start AP connection: {connection_name}')
        print('AP started')
        self.on_ap_started(self.config.ap.ssid)

    async def read_wifi_config(self, iface_name: str) -> tuple[WiFi | None, list[WiFi]]:
        ap, stations = self.netplan_config.read(iface_name)
        print(f'Read WiFi configuration - AP: {ap}, Stations: {stations}')
        return ap, stations

    async def write_wifi_config(self, iface_name: str, ap_config: WiFi, station_configs: list[WiFi]) -> None:
        self.netplan_config.write(iface_name, ap_config, station_configs)
        print(f'Written WiFi configuration - AP: {ap_config}, Stations: {station_configs}')
        await self.netplan_config.apply()
        print('Netplan configuration applied, waiting for NetworkManager...')
        if not await self.network_manager.wait_until_ready():
            raise RuntimeError('NetworkManager did not become ready after netplan apply')
        print('NetworkManager is ready')

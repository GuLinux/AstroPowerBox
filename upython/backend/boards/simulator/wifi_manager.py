import asyncio
from protocols.config import Config
import protocols.wifi_manager
from boards.cpython.json_config_storage import JsonConfigStorage
import json


class SimulatorWiFiManager(protocols.wifi_manager.WiFiManager):
    def __init__(self, config: Config):
        super().__init__(config)
        self.config = config
        with open(JsonConfigStorage.CONFIG_FILE_PATH, 'r') as config_file:
            self.stations = json.load(config_file)['stations']
        print(self.stations)

    def scan(self) -> list[str]:
        return [station['ssid'] for station in self._sorted_available_stations()]

    def _sorted_available_stations(self) -> list[dict]:
        available_stations = [st for st in self.stations if st.get('sim_available', True)]
        return sorted(available_stations, key=lambda st: st.get('sim_rssi', 0))


    async def connect_stations(self, connect_ap_on_failure: bool = True):
        self.on_connecting()
        print('Connecting to WiFi stations...')
        print(f'Configured stations: {self.config.stations}')
        available_stations = self._sorted_available_stations()
        if not available_stations:
            print('No stations configured, skipping connection')
            print('Starting AP mode...')
            await self.start_ap()
            return

        for station in available_stations:
            print(f'Attempting to connect to station: {station['ssid']}')
            await asyncio.sleep(station.get('sim_delay', 1))  # Simulate connection attempt
            if station.get('sim_connection_allowed', True):
                print(f'Connected to station: {station['ssid']}')
                self.on_station_connected(station['ssid'])
                return
            else:
                print('Skipping station')
        print('Failed to connect to any station')
        if connect_ap_on_failure:
            print('Starting AP mode...')
            await self.start_ap()
        pass

    async def start_ap(self):
        print(f'Starting AP with SSID: {self.config.ap.ssid}')
        await asyncio.sleep(1)  # Simulate AP startup
        print('AP started')
        self.on_ap_started(self.config.ap.ssid)

    def set_hostname(self):
        print(f'Setting hostname to {self.config.ap.ssid}')



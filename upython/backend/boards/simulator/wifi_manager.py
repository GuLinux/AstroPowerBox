import asyncio
from config import Config, WiFi
import json


class SimulatorWiFiManager:
    def __init__(self, config: Config):
        self.config = config
        with open(self.config.storage.config_file, 'r') as config_file:
            self.stations = json.load(config_file)['stations']
        print(self.stations)

    def scan(self) -> list[str]:
        available_stations = [st for st in self.stations if st.get('sim_available', True)]
        return sorted(available_stations, key=lambda st: st.get('sim_rssi', 0))


    async def connect_stations(self, connect_ap_on_failure: bool = True):
        print('Connecting to WiFi stations...')
        print(f'Configured stations: {self.config.stations}')
        available_stations = self.scan()
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

    def set_hostname(self):
        print(f'Setting hostname to {self.config.ap.ssid}')



import asyncio
from config import Config, WiFi


class SimulatorWiFiManager:
    def __init__(self, config: Config):
        self.config = config

    def scan(self) -> list[str]:
        return ['Network 1', 'Network 2', 'Network 3']

    async def connect_stations(self, connect_ap_on_failure: bool = True):
        print('Connecting to WiFi stations...')
        print(f'Configured stations: {self.config.stations}')
        if not self.config.stations:
            print('No stations configured, skipping connection')
            print('Starting AP mode...')
            await self.start_ap()
            return

        for station in self.config.stations:
            print(f'Attempting to connect to station: {station.ssid}')
            print(f'Connect? [y/n]')
            if input().lower().strip() == 'y':
                await asyncio.sleep(1)  # Simulate connection attempt
                print(f'Connected to station: {station.ssid}')
                return
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



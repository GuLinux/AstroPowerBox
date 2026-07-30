import protocols.wifi_manager
from protocols.config import Config
from boards.cpython.net.networkmanager import NetworkManager


class WiFiManager(protocols.wifi_manager.WiFiManager):
    def __init__(self, config: Config):
        super().__init__(config)
        self.config = config
        self.network_manager = NetworkManager()
        self._prefix = 'apb-'

    async def connect_stations(self, connect_ap_on_failure: bool = True):
        device = await self.network_manager.get_wifi_device()
        if not device:
            print('No WiFi device found, skipping station connection')
            if connect_ap_on_failure:
                await self.start_ap()
            return

        if not self.config.stations:
            print('No stations configured, starting AP mode')
            if connect_ap_on_failure:
                await self.start_ap()
            return

        # If currently connected to AP, disconnect it first
        current_connection = await self.network_manager.get_active_connection_name(device)
        if current_connection == f'{self._prefix}ap':
            print(f'Currently connected to AP, disconnecting to try stations...')
            await self.network_manager.disconnect_device(device)

        self.on_connecting()
        if not await self.network_manager.apply_wifi_config(device, self.config.ap, self.config.stations, self._prefix):
            print('Failed to apply WiFi configuration')
            if connect_ap_on_failure:
                await self.start_ap()
            return

        print('Connecting to best available WiFi station...')
        if await self.network_manager.connect_device(device):
            connection_name = await self.network_manager.get_active_connection_name(device)
            ssid = connection_name.removeprefix(self._prefix) if connection_name and connection_name.startswith(self._prefix) else 'unknown'
            print(f'Connected to station: {ssid}')
            self.on_station_connected(ssid)
            return

        print('Failed to connect to any station')
        if connect_ap_on_failure:
            await self.start_ap()

    async def start_ap(self):
        device = await self.network_manager.get_wifi_device()
        if not device:
            raise RuntimeError('No WiFi device found, cannot start AP mode')

        if not await self.network_manager.apply_wifi_config(device, self.config.ap, self.config.stations, self._prefix):
            raise RuntimeError('Failed to apply WiFi configuration')
        
        ap_connection_name = f'{self._prefix}ap'
        print(f'Starting AP with SSID: {self.config.ap.ssid}, connection name: {ap_connection_name}')
        if not await self.network_manager.connect_station(ap_connection_name, device):
            raise RuntimeError(f'Failed to start AP connection: {ap_connection_name}')
        print('AP started')
        self.on_ap_started(self.config.ap.ssid)

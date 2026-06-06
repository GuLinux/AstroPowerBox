import protocols.wifi_manager
from protocols.config import Config
import asyncio
from enum import Enum


class WiFiManager(protocols.wifi_manager.WiFiManager):
    class NmConnectionStatus(Enum):
        NM_DEVICE_STATE_UNKNOWN = (0, "the device's state is unknown")
        NM_DEVICE_STATE_UNMANAGED = (10, "the device is not managed by NetworkManager")
        NM_DEVICE_STATE_UNAVAILABLE = (20, "the device is unavailable")
        NM_DEVICE_STATE_DISCONNECTED = (30, "the device is disconnected")
        NM_DEVICE_STATE_PREPARE = (40, "the device is preparing to connect")
        NM_DEVICE_STATE_CONFIG = (50, "the device is configuring the connection")
        NM_DEVICE_STATE_NEED_AUTH = (60, "the device needs authentication to connect")
        NM_DEVICE_STATE_IP_CONFIG = (70, "the device is obtaining an IP address")
        NM_DEVICE_STATE_ACTIVATED = (100, "the device is connected and has an IP address")
        NM_DEVICE_STATE_DEACTIVATING = (110, "the device is disconnecting")
        NM_DEVICE_STATE_FAILED = (120, "the device failed to connect")

        @property
        def code(self) -> int:
            return self.value[0]

        @property
        def description(self) -> str:
            return self.value[1]


    def __init__(self, config: Config):
        super().__init__(config)
        self.config = config

    async def connect_stations(self, connect_ap_on_failure: bool = True):
        wifi_device = next((d for d in await self._get_devices() if d['TYPE'] == 'wifi'), None)
        if not wifi_device:
            print('No WiFi device found, skipping connection')
            print('Starting AP mode...')
            await self.start_ap()
            return
        self.on_connecting()
        print('Connecting to WiFi stations...')
        connected = await self._autoconnect_device(wifi_device['DEVICE'])
        if connected:
            print('Connected to WiFi station')
            self.on_station_connected(wifi_device['DEVICE'])
        else:
            print('Failed to connect to WiFi station')
            if connect_ap_on_failure:
                print('Starting AP mode...')
                await self.start_ap()

    async def start_ap(self):
        ap_connection = await self._get_ap_connection()
        if not ap_connection:
            raise RuntimeError('No AP connection found, cannot start AP mode')
        print(f'Starting AP with SSID: {self.config.ap.ssid}')
        await self.__run_nmcli('connection', 'up', ap_connection['NAME'])
        print('AP started')
        self.on_ap_started(self.config.ap.ssid)

    async def _autoconnect_device(self, device: str) -> bool:
        print(f'Attempting to autoconnect device {device}...')
        await self.__run_nmcli('device', 'connect', device)
        for _ in range(30):
            status = await self._get_device_status(device)
            print(f'Device {device} status: {status.description}')
            if status == self.NmConnectionStatus.NM_DEVICE_STATE_ACTIVATED:
                return True
            elif status in (self.NmConnectionStatus.NM_DEVICE_STATE_FAILED, self.NmConnectionStatus.NM_DEVICE_STATE_UNAVAILABLE):
                return False
            await asyncio.sleep(1)
        print(f'Timed out while waiting for device {device} to connect')
        return False

    async def _get_devices(self):
        return await self.__run_nmcli_fields(('DEVICE', 'TYPE'), 'device')

    async def _get_ap_connection(self):
        connections = await self._get_wifi_connections()
        return next((conn for conn in connections if conn['MODE'] == 'ap'), None)

    async def _get_wifi_connections(self):
        all_connections = await self.__run_nmcli_fields(('NAME', 'UUID', 'TYPE'), 'connection', 'show')
        wifi_connections = [conn for conn in all_connections if conn['TYPE'] == '802-11-wireless']
        for conn in wifi_connections:
            conn_details = await self.__run_nmcli_fields(('802-11-wireless.mode', '802-11-wireless.ssid'), 'connection', 'show', conn['UUID'])
            conn.update(conn_details[0])
        return wifi_connections

    async def _get_device_status(self, device: str) -> NmConnectionStatus:
        status = await self.__run_nmcli_fields(('GENERAL.STATE',), 'device', 'show', device)
        if not status:
            raise RuntimeError(f'No status found for device {device}')
        status = int(status[0]['GENERAL.STATE'].split(' ')[0])
        status = next((s for s in self.NmConnectionStatus if s.code == status), None)
        if not status:
            raise RuntimeError(f'Unknown status code {status} for device {device}')
        return status

    async def __run_nmcli_fields(self, fields: tuple[str,...], command: str, *args) -> list[dict[str, str]]:
        arguments = ['-g', ','.join(fields), command, *args]
        stdout = await self.__run_nmcli(*arguments)
        return [dict(zip(fields, tuple(line.split(':')))) for line in stdout.splitlines()]

    async def __run_nmcli(self, *args) -> str:
        arguments = ['nmcli', *args]
        process = await asyncio.create_subprocess_exec(*arguments, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
        stdout, stderr = await process.communicate()
        if process.returncode != 0:
            raise RuntimeError(f'Command {" ".join(arguments)} failed with error: {stderr.decode()}')
        return stdout.decode()
    
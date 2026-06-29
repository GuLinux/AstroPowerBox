import asyncio
from enum import Enum


class NetworkManager:
    class DeviceState(Enum):
        UNKNOWN = (0, "the device's state is unknown")
        UNMANAGED = (10, "the device is not managed by NetworkManager")
        UNAVAILABLE = (20, "the device is unavailable")
        DISCONNECTED = (30, "the device is disconnected")
        PREPARE = (40, "the device is preparing to connect")
        CONFIG = (50, "the device is configuring the connection")
        NEED_AUTH = (60, "the device needs authentication to connect")
        IP_CONFIG = (70, "the device is obtaining an IP address")
        ACTIVATED = (100, "the device is connected and has an IP address")
        DEACTIVATING = (110, "the device is disconnecting")
        FAILED = (120, "the device failed to connect")

        @property
        def code(self) -> int:
            return self.value[0]

        @property
        def description(self) -> str:
            return self.value[1]

    async def get_wifi_device(self) -> str | None:
        devices = await self._run_nmcli_fields(('DEVICE', 'TYPE'), 'device')
        wifi_device = next((device for device in devices if device['TYPE'] == 'wifi'), None)
        return wifi_device['DEVICE'] if wifi_device else None

    async def connect_station(self, connection_name: str, device: str | None = None) -> bool:
        arguments = ['connection', 'up', connection_name]
        if device:
            arguments.extend(['ifname', device])
        try:
            await self._run_nmcli(*arguments)
        except RuntimeError as error:
            print(error)
            return False
        if not device:
            return True
        return await self.wait_for_device_state(device, self.DeviceState.ACTIVATED)

    async def connect_device(self, device: str) -> bool:
        try:
            await self._run_nmcli('device', 'connect', device)
        except RuntimeError as error:
            print(error)
            return False
        return await self.wait_for_device_state(device, self.DeviceState.ACTIVATED)

    async def get_active_connection_name(self, device: str) -> str | None:
        result = await self._run_nmcli_fields(('GENERAL.CONNECTION',), 'device', 'show', device)
        if not result:
            return None
        return result[0].get('GENERAL.CONNECTION') or None

    async def start_ap(self, connection_name: str, device: str | None = None) -> bool:
        return await self.connect_station(connection_name, device)

    async def wait_for_device_state(
        self,
        device: str,
        expected_state: DeviceState,
        attempts: int = 30,
        delay: float = 1,
    ) -> bool:
        for _ in range(attempts):
            status = await self.get_device_status(device)
            print(f'Device {device} status: {status.description}')
            if status == expected_state:
                return True
            if status in (self.DeviceState.FAILED, self.DeviceState.UNAVAILABLE):
                return False
            await asyncio.sleep(delay)
        return False

    async def get_device_status(self, device: str) -> DeviceState:
        status = await self._run_nmcli_fields(('GENERAL.STATE',), 'device', 'show', device)
        if not status:
            raise RuntimeError(f'No status found for device {device}')
        status_code = int(status[0]['GENERAL.STATE'].split(' ')[0])
        state = next((state for state in self.DeviceState if state.code == status_code), None)
        if not state:
            raise RuntimeError(f'Unknown NetworkManager device state: {status_code}')
        return state

    async def wait_until_ready(self, attempts: int = 30, delay: float = 1.0) -> bool:
        for _ in range(attempts):
            try:
                await self._run_nmcli('general', 'status')
                return True
            except RuntimeError:
                await asyncio.sleep(delay)
        return False

    async def _run_nmcli_fields(self, fields: tuple[str, ...], command: str, *args) -> list[dict[str, str]]:
        stdout = await self._run_nmcli('-g', ','.join(fields), command, *args)
        return [dict(zip(fields, tuple(line.split(':')))) for line in stdout.splitlines()]

    async def _run_nmcli(self, *args) -> str:
        arguments = ['nmcli', *args]
        process = await asyncio.create_subprocess_exec(
            *arguments,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        stdout, stderr = await process.communicate()
        if process.returncode != 0:
            raise RuntimeError(f'Command {" ".join(arguments)} failed with error: {stderr.decode()}')
        return stdout.decode()

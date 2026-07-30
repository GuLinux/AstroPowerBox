import asyncio
from types import SimpleNamespace

from boards.cpython.wifi_manager import WiFiManager
from wifi import WiFi


class _NetworkManager:
    def __init__(
        self,
        device='wlan0',
        apply_result=True,
        connect_result=True,
        active_connection='apb-Home',
        active_connections=None,
    ):
        self.device = device
        self.apply_result = apply_result
        self.connect_result = connect_result
        self.active_connection = active_connection
        self.active_connections = active_connections or []
        self.calls = []

    async def get_wifi_device(self):
        self.calls.append(('get_wifi_device',))
        return self.device

    async def get_active_connection_name(self, device):
        self.calls.append(('get_active_connection_name', device))
        if self.active_connections:
            return self.active_connections.pop(0)
        return self.active_connection

    async def disconnect_device(self, device):
        self.calls.append(('disconnect_device', device))
        return True

    async def apply_wifi_config(self, device, ap, stations, prefix):
        self.calls.append(('apply_wifi_config', device, ap, stations, prefix))
        return self.apply_result

    async def connect_device(self, device):
        self.calls.append(('connect_device', device))
        return self.connect_result

    async def connect_station(self, connection_name, device):
        self.calls.append(('connect_station', connection_name, device))
        return self.connect_result


def _new_manager(stations=None):
    config = SimpleNamespace(
        ap=WiFi('AstroPowerBox', 'secret'),
        stations=stations if stations is not None else [WiFi('Home', 'station-secret')],
    )
    manager = WiFiManager(config)
    return manager, config


def test_connect_stations_starts_ap_when_no_device():
    manager, _ = _new_manager()
    manager.network_manager = _NetworkManager(device=None)
    ap_starts = []

    async def start_ap():
        ap_starts.append(True)

    manager.start_ap = start_ap

    asyncio.run(manager.connect_stations())

    assert ap_starts == [True]


def test_connect_stations_replaces_active_ap_and_reports_station_connection():
    manager, _ = _new_manager()
    network_manager = _NetworkManager(active_connections=['apb-ap', 'apb-Home'])
    manager.network_manager = network_manager
    connecting = []
    connected = []
    manager.on_connecting = lambda: connecting.append(True)
    manager.on_station_connected = connected.append

    asyncio.run(manager.connect_stations())

    assert ('disconnect_device', 'wlan0') in network_manager.calls
    assert ('connect_device', 'wlan0') in network_manager.calls
    assert connecting == [True]
    assert connected == ['Home']


def test_connect_stations_starts_ap_when_wifi_configuration_fails():
    manager, _ = _new_manager()
    manager.network_manager = _NetworkManager(active_connection=None, apply_result=False)
    ap_starts = []

    async def start_ap():
        ap_starts.append(True)

    manager.start_ap = start_ap

    asyncio.run(manager.connect_stations())

    assert ap_starts == [True]


def test_start_ap_connects_and_reports_configured_ssid():
    manager, config = _new_manager()
    network_manager = _NetworkManager()
    manager.network_manager = network_manager
    started = []
    manager.on_ap_started = started.append

    asyncio.run(manager.start_ap())

    assert ('connect_station', 'apb-ap', 'wlan0') in network_manager.calls
    assert started == [config.ap.ssid]

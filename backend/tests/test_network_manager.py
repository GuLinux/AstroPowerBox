import asyncio

from boards.cpython.net.networkmanager import NetworkManager


def test_nmcli_fields_parses_machine_readable_output(monkeypatch):
    manager = NetworkManager()

    async def run_nmcli(*args):
        assert args == ('-g', 'DEVICE,TYPE', 'device')
        return 'wlan0:wifi\neth0:ethernet\n'

    monkeypatch.setattr(manager, '_run_nmcli', run_nmcli)

    fields = asyncio.run(manager._run_nmcli_fields(('DEVICE', 'TYPE'), 'device'))

    assert fields == [
        {'DEVICE': 'wlan0', 'TYPE': 'wifi'},
        {'DEVICE': 'eth0', 'TYPE': 'ethernet'},
    ]


def test_wait_for_device_state_stops_on_expected_or_terminal_state(monkeypatch):
    manager = NetworkManager()

    async def active_status(_device):
        return manager.DeviceState.ACTIVATED

    monkeypatch.setattr(manager, 'get_device_status', active_status)
    assert asyncio.run(manager.wait_for_device_state('wlan0', manager.DeviceState.ACTIVATED)) is True

    async def failed_status(_device):
        return manager.DeviceState.FAILED

    monkeypatch.setattr(manager, 'get_device_status', failed_status)
    assert asyncio.run(manager.wait_for_device_state('wlan0', manager.DeviceState.ACTIVATED)) is False


def test_connect_device_returns_false_when_nmcli_fails(monkeypatch):
    manager = NetworkManager()

    async def run_nmcli(*_args):
        raise RuntimeError('nmcli failed')

    monkeypatch.setattr(manager, '_run_nmcli', run_nmcli)

    assert asyncio.run(manager.connect_device('wlan0')) is False


def test_add_or_update_connection_uses_open_network_settings(monkeypatch):
    manager = NetworkManager()
    commands = []

    async def run_nmcli(*args):
        commands.append(args)
        if args[:3] == ('connection', 'show', 'apb-open'):
            raise RuntimeError('connection not found')
        return ''

    monkeypatch.setattr(manager, '_run_nmcli', run_nmcli)

    result = asyncio.run(
        manager.add_or_update_connection(
            'apb-open',
            'OpenNetwork',
            psk='',
            device='wlan0',
            ipv4_method='shared',
            ipv6_disabled=True,
        )
    )

    assert result is True
    assert (
        'connection',
        'add',
        'type',
        'wifi',
        'con-name',
        'apb-open',
        'ssid',
        'OpenNetwork',
        'ifname',
        'wlan0',
        'wifi-sec.key-mgmt',
        'none',
        'ipv4.method',
        'shared',
        'ipv6.method',
        'ignore',
    ) in commands
    assert ('connection', 'modify', 'apb-open', 'connection.autoconnect-priority', '0') in commands


def test_list_connections_filters_by_prefix(monkeypatch):
    manager = NetworkManager()

    async def fields(_fields, *_args):
        return [{'NAME': 'apb-ap'}, {'NAME': 'home'}, {'NAME': 'apb-office'}]

    monkeypatch.setattr(manager, '_run_nmcli_fields', fields)

    assert asyncio.run(manager.list_connections('apb-')) == ['apb-ap', 'apb-office']

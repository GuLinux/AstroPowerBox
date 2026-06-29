from wifi import WiFi
import os
import asyncio
import yaml
### Netplan Structure:
# network:
#   version: 2
#   renderer: NetworkManager
#   wifis:
#     wlxe0469a1904b1:
#       dhcp4: yes
#       access-points:
#         "Tardis":
#           password: "*********"
#           networkmanager:
#             name: "APB-Tardis"
#             passthrough:
#               "connection.autoconnect-priority": 100
#         "AstroPowerBox":
#           password: "*********"
#           mode: "ap"
#           networkmanager:
#             name: "APB-AstroPowerBox-AP"

class NetPlanConfig:
    AP_CONNECTION_NAME = 'AstroPowerBox-AP'
    STATION_CONNECTION_PREFIX = 'APB-'

    def __init__(self, config_file: str = '/etc/netplan/01-netcfg.yaml'):
        self.config_file = config_file

    @classmethod
    def ap_connection_name(cls) -> str:
        return cls.AP_CONNECTION_NAME

    @classmethod
    def station_connection_name(cls, ssid: str) -> str:
        return f'{cls.STATION_CONNECTION_PREFIX}{ssid}'

    def read(self, iface_name: str) -> tuple[WiFi | None, list[WiFi]]:
        if not os.path.exists(self.config_file):
            print(f'Netplan config file {self.config_file} not found, returning empty config')
            return None, []
        with open(self.config_file, 'r') as f:
            data = yaml.safe_load(f) or {}
        wifi_config = data.get('network', {}).get('wifis', {}).get(iface_name, {}).get('access-points', {})
        ap = None
        stations = []
        for ssid, station in wifi_config.items():
            if station is None:
                station = {}
            if station.get('mode') != 'ap':
                password = station.get('password', '')
                stations.append(WiFi(ssid=ssid, psk=password))
            elif station.get('networkmanager', {}).get('name', '') == self.ap_connection_name():
                ap = WiFi(ssid=ssid, psk=station.get('password', ''))

        return ap, stations

    def write(self, iface_name: str, ap: WiFi, stations: list[WiFi]):
        netplan_obj = {
            'network': {
                'version': 2,
                'renderer': 'NetworkManager',
                'wifis': {
                    iface_name: {
                        'dhcp4': 'yes',
                        'access-points': {
                            ap.ssid: {
                                'mode': 'ap',
                                'password': ap.psk,
                                'networkmanager': {
                                    'name': self.ap_connection_name(),
                                    'passthrough': {
                                        'connection.autoconnect-priority': 0
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        for station in stations:
            netplan_obj['network']['wifis'][iface_name]['access-points'][station.ssid] = {
                'password': station.psk,
                'networkmanager': {
                    'name': self.station_connection_name(station.ssid),
                    'passthrough': {
                        'connection.autoconnect-priority': 100
                    }
                }
            }
        with open(self.config_file, 'w') as f:
            yaml.dump(netplan_obj, f)

    async def apply(self):
        process = await asyncio.create_subprocess_exec(
            'sudo',
            'netplan',
            'apply',
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        stdout, stderr = await process.communicate()
        if process.returncode != 0:
            raise RuntimeError(f'Failed to apply netplan configuration: {stderr.decode()}')
        if stdout:
            print(stdout.decode())

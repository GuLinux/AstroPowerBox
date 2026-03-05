import uasyncio as asyncio
import network
import binascii
import typing
import protocols.wifi_manager

if typing.TYPE_CHECKING:
    from config import Config

class ScanResult:
    def __init__(self, ssid: bytes, bssid: bytes, channel: int, rssi: int, security: int, hidden: bool):
        self.ssid = ssid.decode()
        self.bssid = bssid
        self.channel = channel
        self.rssi = rssi
        self.security = security
        self.hidden = hidden

    @property
    def bssid_str(self) -> str:
        return binascii.hexlify(self.bssid, ':').decode()

    @property
    def security_str(self) -> str:
        if self.security == 0:
            return 'OPEN'
        elif self.security == 1:
            return 'WEP'
        elif self.security == 2:
            return 'WPA-PSK'
        elif self.security == 3:
            return 'WPA2-PSK'
        elif self.security == 4:
            return 'WPA/WPA2-PSK'
        else:
            return 'UNKNOWN'
        
    def __str__(self):
        return f'ScanResult(ssid={self.ssid}, bssid={self.bssid_str}, channel={self.channel}, rssi={self.rssi}, security={self.security_str}, hidden={self.hidden})'
    
    def __repr__(self):
        return self.__str__()

class ESPWiFiManager(protocols.wifi_manager.WiFiManager):
    def __init__(self, config: Config):
        super().__init__(config)
        self.config = config
        self.station_wlan = network.WLAN(network.STA_IF)
        self.ap_wlan = network.WLAN(network.AP_IF)

    def scan(self) -> list[ScanResult]:
        self.station_wlan.active(True)
        raw_results = self.station_wlan.scan()
        return [ScanResult(*result) for result in raw_results]

    async def connect_stations(self, connect_ap_on_failure: bool = True):
        self.on_connecting()
        self.set_hostname()
        self.ap_wlan.active(False)  # Disable AP mode while trying to connect to stations
        print('Connecting to WiFi stations...')
        if not self.config.stations:
            print('No configured stations, starting in AP mode')
            await self.start_ap()
            return

        print('Configured stations:')
        for wifi in self.config.stations:
            print(f'  {wifi.ssid}')
        available_networks = self.scan()
        available_networks.sort(key=lambda x: x.rssi, reverse=True)  # Sort by signal strength
        print('Available networks:')
        available_configured_networks = []
        for network in available_networks:
            print(network)
            wifi = [st for st in self.config.stations if st.ssid == network.ssid]
            if wifi and not wifi[0] in available_configured_networks:
                available_configured_networks.append(wifi[0])

        print('Available configured networks:')
        for network in available_configured_networks:
            print(network)

        for wifi in available_configured_networks:
            wlan = self.station_wlan
            wlan.active(True)
            wlan.connect(wifi.ssid, wifi.psk)
            for _ in range(10):
                if wlan.isconnected():
                    print(f'Connected to {wifi.ssid}')
                    self.on_station_connected(wifi.ssid)
                    return
                await asyncio.sleep(1)  # Non-blocking wait
            print(f'Failed to connect to {wifi.ssid}')

        if connect_ap_on_failure:
            print('Failed to connect to any station, starting AP mode')
            await self.start_ap()

    async def start_ap(self):
        self.set_hostname()
        self.station_wlan.active(False)  # Disable station mode
        ap = self.config.ap
        self.ap_wlan.active(True)
        self.ap_wlan.config(ssid=ap.ssid, key=ap.psk, security=3)
        print(f'AP started with SSID: {ap.ssid}')
        self.on_ap_started(ap.ssid)

    def set_hostname(self):
        network.hostname(self.config.ap.ssid)



import json
from config import Config
import protocols.wifi_manager
import protocols.config_storage
from status_led import StatusLed
from board_compat import ConfigStorage, WiFiManager, pinout_config_path, gpio

class Board:
    config_storage: protocols.config_storage.ConfigStorage
    config: Config
    wifi_manager: protocols.wifi_manager.WiFiManager
    pinout_config: dict

    async def start(self):
        await self.status_led.start()

    def __init__(self):
        self.config_storage = ConfigStorage()
        self.config = Config(self.config_storage)
        self.wifi_manager = WiFiManager(self.config)
        with open(pinout_config_path, 'r') as pinout_config_file:
            print(f'Loading pinout file: {pinout_config_path}')
            self.pinout_config = json.load(pinout_config_file)
        self.status_led = StatusLed(self.__load_output(self.pinout_config['status_led']), self.config)
        self.wifi_manager.on_connecting = lambda: self.status_led.wifi_connecting()
        self.wifi_manager.on_station_connected = lambda _: self.status_led.status_ok()
        self.wifi_manager.on_ap_started = lambda _: self.status_led.wifi_failed()
        

    def __load_output(self, config: dict):
        if config['type'] == 'pwm':
            return gpio.PWMOutputPin(config['pin'])
        return gpio.DigitalOutputPin(config['pin'])



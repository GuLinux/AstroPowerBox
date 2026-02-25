import sys
from config import Config
import typing
if typing.TYPE_CHECKING:
    from protocols.wifi_manager import WiFiManager
import protocols.config_storage
if sys.implementation.name == 'micropython':
    if sys.platform == 'esp32':
        import uasyncio as asyncio
        from boards.esp32.wifi_manager import ESPWiFiManager as WiFiManager
        from boards.esp32.config_storage import ESPConfigStorage as ConfigStorage
    else:
        raise RuntimeError('Unsupported micropython platform')
elif sys.implementation.name == 'cpython':
    import os
    import asyncio
    from boards.cpython.json_config_storage import JsonConfigStorage as ConfigStorage
    if os.environ.get('SIMULATOR', '0') == '1':
        from boards.simulator.wifi_manager import SimulatorWiFiManager as WiFiManager
else:
    raise RuntimeError(f'Unsupported python platform: {sys.implementation.name}')

class Board:
    config_storage: protocols.config_storage.ConfigStorage
    config: Config
    wifi_manager: WiFiManager

    def __init__(self):
        self.config_storage = ConfigStorage()
        self.config = Config(self.config_storage)
        self.wifi_manager = WiFiManager(self.config)
        



import sys

if sys.implementation.name == 'micropython':
    if sys.platform == 'esp32':
        import uasyncio as asyncio
        from boards.esp32.wifi_manager import ESPWiFiManager as WiFiManager
        from boards.esp32.config_storage import ESPConfigStorage as ConfigStorage
        pinout_config_path = '/pinout.json'
    else:
        raise RuntimeError('Unsupported micropython platform')
elif sys.implementation.name == 'cpython':
    import os
    import asyncio
    from boards.cpython.json_config_storage import JsonConfigStorage as ConfigStorage
    if os.environ.get('SIMULATOR', '0') == '1':
        from boards.simulator.wifi_manager import SimulatorWiFiManager as WiFiManager
        import boards.simulator.gpio as gpio
    pinout_config_path = os.environ.get('PINOUT_CONFIG_PATH', 'pinout.json')
else:
    raise RuntimeError(f'Unsupported python platform: {sys.implementation.name}')


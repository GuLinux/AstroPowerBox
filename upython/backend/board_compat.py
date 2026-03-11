import sys

if sys.implementation.name == 'micropython':
    if sys.platform == 'esp32':
        import uasyncio as asyncio
        from boards.esp32.wifi_manager import ESPWiFiManager as WiFiManager
        from boards.esp32.config_storage import ESPConfigStorage as ConfigStorage
        import boards.esp32.gpio as gpio
        from board_vars import board_name
        pinout_config_path = f'/pinout_{board_name}.json'
        server_port = 80
        server_debug = False
    else:
        raise RuntimeError('Unsupported micropython platform')
elif sys.implementation.name == 'cpython':
    import os
    import asyncio
    from boards.cpython.json_config_storage import JsonConfigStorage as ConfigStorage
    if os.environ.get('SIMULATOR', '0') == '1':
        from boards.simulator.wifi_manager import SimulatorWiFiManager as WiFiManager
        import boards.simulator.gpio as gpio
    pinout_config_path = os.environ.get('PINOUT_CONFIG_PATH', 'config_files/pinout.json')
    server_port = int(os.environ.get('PORT', '80'))
    server_debug = os.environ.get('DEBUG', 0) in ['1', 'true']
else:
    raise RuntimeError(f'Unsupported python platform: {sys.implementation.name}')


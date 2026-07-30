from gpiozero.pins.lgpio import LGPIOFactory
from gpiozero.pins.pi import PiBoardInfo
from gpiozero import Device, PWMLED, Button
import time

class OrangePiFactory(LGPIOFactory):
    def __init__(self):
        self._info = self._get_pi_info()

    def _get_pi_info(self):
        # We return a dummy PiBoardInfo to satisfy the library logic
        return PiBoardInfo(
            revision='0000',
            model='Orange Pi 5',
            pcb_revision='1.0',
            released='2024',
            soc='RK3588S',
            manufacturer='Orange Pi',
            memory=16384,
            storage='Unknown',
            usb=3,
            ethernet=1,
            usb3=0,
            eth_speed=0,
            board=None,
            wifi=True,
            bluetooth=True,
            csi=1,
            dsi=1,
            headers={} # Empty headers forces the factory to use raw chip:offset strings
        )


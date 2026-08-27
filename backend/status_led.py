from board_compat import asyncio
from protocols.gpio import DigitalOutputPin, PWMOutputPin
from protocols.config import Config
import logging

logger = logging.getLogger(__name__)

class StatusLed:
    def __init__(self, output_pin: DigitalOutputPin | PWMOutputPin, config: Config):
        self.gpio_output = output_pin
        self.config = config
        self.pattern = [(False, 1)]

    async def start(self):
        logger.info(f'Starting status led task: gpio_output={self.gpio_output}')
        asyncio.create_task(self.__loop())

    async def __loop(self):
        while True:
            for state, time in self.pattern:
                self._set_led(state)
                await asyncio.sleep(time)

    def _set_led(self, state):
        self.gpio_output.duty = self.config.status_led_duty if state else 0.0 

    def wifi_connecting(self):
        logger.debug('Status led: wifi connecting')
        self.pattern = [(True, 0.2), (False, 0.2)]

    def status_ok(self):
        logger.debug('Status led: status ok')
        self.pattern = [(True, 2), (False, 0.4)]

    def wifi_failed(self):
        logger.debug('Status led: wifi failed')
        self.pattern = [(True, 0.4), (False, 0.4)] * 3 + [(False, 2)]

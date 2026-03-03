from board_compat import asyncio

class StatusLed:
    def __init__(self, gpio_output):
        self.gpio_output = gpio_output
        self.pattern = [(False, 1)]

    async def start(self):
        print('Starting status led task')
        asyncio.create_task(self.__loop())

    async def __loop(self):
        while True:
            for state, time in self.pattern:
                self._set_led(state)
                await asyncio.sleep(time)

    def _set_led(self, state):
        if self.gpio_output.is_pwm:
            self.gpio_output.duty = 1 if state else 0
        else:
            self.gpio_output.value = state


    def wifi_connecting(self):
        self.pattern = [(True, 0.5), (False, 0.5)]

    def status_ok(self):
        pass

    def wifi_failed(self):
        pass

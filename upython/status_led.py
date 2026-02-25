class StatusLed:
    def __init__(self, gpio_output):
        self.gpio_output = gpio_output

    def wifi_connecting(self):
        pass

    def status_ok(self):
        pass

    def wifi_failed(self):
        pass

from types import SimpleNamespace

import status_led


class _OutputPin:
    def __init__(self):
        self.duty = None


def test_status_led_sets_configured_duty_for_on_state():
    pin = _OutputPin()
    led = status_led.StatusLed(pin, SimpleNamespace(status_led_duty=0.35))

    led._set_led(True)
    assert pin.duty == 0.35

    led._set_led(False)
    assert pin.duty is False


def test_status_led_selects_expected_patterns():
    led = status_led.StatusLed(_OutputPin(), SimpleNamespace(status_led_duty=1.0))

    led.wifi_connecting()
    assert led.pattern == [(True, 0.2), (False, 0.2)]

    led.status_ok()
    assert led.pattern == [(True, 2), (False, 0.4)]

    led.wifi_failed()
    assert led.pattern == [(True, 0.4), (False, 0.4)] * 3 + [(False, 2)]


def test_status_led_start_creates_background_task(monkeypatch):
    led = status_led.StatusLed(_OutputPin(), SimpleNamespace(status_led_duty=1.0))
    tasks = []

    def create_task(coroutine):
        tasks.append(coroutine)

    monkeypatch.setattr(status_led.asyncio, 'create_task', create_task)

    import asyncio

    asyncio.run(led.start())

    assert len(tasks) == 1
    tasks[0].close()

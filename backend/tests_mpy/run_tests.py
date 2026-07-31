import sys

from tests_mpy import test_core


class SkipTest(Exception):
    pass


def _run(name, func):
    try:
        func()
        print('PASS', name)
        return True
    except SkipTest as ex:
        print('SKIP', name, '-', ex)
        return True
    except Exception as ex:
        print('FAIL', name, '-', ex)
        return False


def main():
    tests = [
        ('test_wifi_requires_ssid', test_core.test_wifi_requires_ssid),
        ('test_wifi_roundtrip', test_core.test_wifi_roundtrip),
        ('test_check_required_key_raises', test_core.test_check_required_key_raises),
        ('test_config_roundtrip_persists_wifi_and_runtime_settings', test_core.test_config_roundtrip_persists_wifi_and_runtime_settings),
    ]

    if sys.implementation.name == 'micropython' and sys.platform == 'esp32':
        from tests_mpy import test_hardware
        tests.append(('test_esp32_runtime_is_available', test_hardware.test_esp32_runtime_is_available))
        tests.append(('test_status_led_pwm_output_on_device', test_hardware.test_status_led_pwm_output_on_device))

    failures = 0
    for name, func in tests:
        if not _run(name, func):
            failures += 1

    print('---')
    print('Total:', len(tests), 'Failed:', failures)
    return failures


if __name__ == '__main__':
    sys.exit(main())

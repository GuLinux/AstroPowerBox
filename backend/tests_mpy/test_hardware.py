def test_esp32_runtime_is_available():
    from machine import unique_id

    if not unique_id():
        raise AssertionError('ESP32 unique ID is empty')


def test_status_led_pwm_output_on_device():
    import json

    try:
        from board_vars import board_name
    except ImportError:
        from tests_mpy.run_tests import SkipTest
        raise SkipTest('board_vars is not deployed')

    from boards.esp32.gpio import PWMOutputPin

    with open('/pinout_{}.json'.format(board_name), 'r') as pinout_file:
        pinout = json.load(pinout_file)

    status_led = pinout.get('status_led')
    if not isinstance(status_led, dict) or status_led.get('type') != 'pwm':
        raise AssertionError('Selected board must define a PWM status LED for HIL testing')

    output = PWMOutputPin(str(status_led['pin']))
    changes = []
    output.on_duty_changed(changes.append)
    output.duty = 0.02
    output.duty = 0.0

    if output.duty != 0.0:
        raise AssertionError('Status LED did not return to off')
    if changes != [0.02, 0.0]:
        raise AssertionError('Status LED duty callbacks did not report both changes')

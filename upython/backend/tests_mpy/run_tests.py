import sys

from tests_mpy import test_core


def _run(name, func):
    try:
        func()
        print('PASS', name)
        return True
    except Exception as ex:
        print('FAIL', name, '-', ex)
        return False


def main():
    tests = [
        ('test_wifi_requires_ssid', test_core.test_wifi_requires_ssid),
        ('test_wifi_roundtrip', test_core.test_wifi_roundtrip),
        ('test_check_required_key_raises', test_core.test_check_required_key_raises),
    ]

    failures = 0
    for name, func in tests:
        if not _run(name, func):
            failures += 1

    print('---')
    print('Total:', len(tests), 'Failed:', failures)

    if failures:
        sys.exit(1)
    sys.exit(0)


if __name__ == '__main__':
    main()

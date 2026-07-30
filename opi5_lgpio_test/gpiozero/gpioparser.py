import re

_GPIO_REGEX='^GPIO(\\d)_([A-D])(\\d)$'

def gpio2line(gpio: str):
    gpio = gpio.upper()
    gpio_match = re.match(_GPIO_REGEX, gpio)
    if not gpio_match:
        raise RuntimeError(f"String {gpio} doesn't match pattern _GPIO_REGEX")
    chip = int(gpio_match.group(1))
    line_offset = (gpio_match.group(2).encode()[0] - b'A'[0]) * 8
    line = int(gpio_match.group(3)) + line_offset
    return chip,line
    
if __name__ == '__main__':
    import sys
    print(gpio2line(sys.argv[1]))

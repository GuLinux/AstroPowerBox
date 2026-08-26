#!/usr/bin/env python3
import sys
import os
project_path = os.path.dirname(os.path.dirname(__file__))
sys.path.insert(1, f'{project_path}/backend')


import boards.cpython.gpio as gpio

chip, line = gpio._parse_gpio_pin(sys.argv[1])
print(f'chip={chip}, line={line}')


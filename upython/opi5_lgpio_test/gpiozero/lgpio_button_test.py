import time
from lgpio_wrapper import Button, LED
from gpioparser import gpio2line
from opi5_ultra_gpio_map import OPI5_ULTRA_GPIO_PINS

# Create instances (using your chip:offset)
led1 = LED(*gpio2line(OPI5_ULTRA_GPIO_PINS['PWM1']))
led2 = LED(*gpio2line(OPI5_ULTRA_GPIO_PINS['PWM2']))
button1 = Button(*gpio2line(OPI5_ULTRA_GPIO_PINS['BTN0']))
button2 = Button(*gpio2line(OPI5_ULTRA_GPIO_PINS['BTN1']))


# Define functions to run on events
def on_press(name, led):
    print(f"Button {name} was pressed! Turning LED ON.")
    led.on()

def on_release(name, led):
    print(f"Button {name} was released! Turning LED OFF.")
    led.off()

# Assign callbacks (Asynchronous)
button1.when_pressed = lambda: on_press('0', led1)
button2.when_pressed = lambda: on_press('1', led2)
button1.when_released = lambda: on_release('0', led1)
button2.when_released = lambda: on_release('1', led2)

print("Async listener active. Press Ctrl+C to stop.")

try:
    while True:
        # The main loop is free to do other things
        print(f"Main loop is still running... button pressed: ")
        time.sleep(2)
except KeyboardInterrupt:
    button1.close()
    button2.close()

// #pragma message "Using config for ESP32 S2 Mini"

#define I2C_SDA_PIN 12
#define I2C_SCL_PIN 13
#define APB_STATUS_LED_PIN 8

// Thermistor PWM pinout. First number in a pair is the PWM pin (output), second thermistor PIN (analog input)
// IMPORTANT: there is no compiler check for pinout size, so be careful.
#define APB_PWM_OUTPUTS_PWM_PINOUT {5,1, Heater}, {2,4, Heater}, {3,7, Heater}
#define APB_PWM_OUTPUTS_SIZE 3
#define APB_PWM_OUTPUTS_TEMP_SENSORS 3
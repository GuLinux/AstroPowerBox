#warning Using config for ESP32 C3 Supermini
#define I2C_SDA_PIN 10
#define I2C_SCL_PIN 9
#define APB_STATUS_LED_PIN 8

// Thermistor PWM pinout. First number in a pair is the PWM pin (output), second thermistor PIN (analog input)
// IMPORTANT: there is no compiler check for pinout size, so be careful.
#define APB_HEATERS_PWM_PINOUT Pinout{3,1}, Pinout{4,0}

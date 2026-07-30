// #pragma message "Using config for ESP32 C3 Supermini"
#define I2C_SDA_PIN 10
#define I2C_SCL_PIN 9
#define APB_STATUS_LED_PIN 8

// Thermistor PWM pinout. First number in a pair is the PWM pin (output), second thermistor PIN (analog input)
// Setting -1 for the thermistor indicates there's no thermistor available.
// IMPORTANT: there is no compiler check for pinout size, so be careful.
#define APB_PWM_OUTPUTS_PWM_PINOUT {3,1,Heater},{4,0,Heater},{20,-1,Heater}
#define APB_PWM_OUTPUTS_SIZE 3
#define APB_PWM_OUTPUTS_TEMP_SENSORS 2
#define ONEBUTTON_USER_BUTTON_1 21
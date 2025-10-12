#define I2C_SDA_PIN 15
#define I2C_SCL_PIN 16
#define APB_STATUS_LED_PIN 4


#define SPI_SCK 12
#define SPI_MISO 13
#define SPI_MOSI 11
#define SPI_SS  10

#define APB_PWM_0_PIN 41
#define APB_PWM_1_PIN 42
#define APB_PWM_2_PIN 46
#define APB_PWM_3_PIN 9
#define APB_PWM_4_PIN 14
#define APB_PWM_5_PIN 21
#define APB_PWM_FAN_PIN 3
#define APB_THERMISTOR_0_PIN 2
#define APB_THERMISTOR_1_PIN 1

#define CH224K_CFG1_PIN 17
#define CH224K_CFG2_PIN 18
#define CH224K_CFG3_PIN 8

#define ONEBUTTON_USER_BUTTON_1 7

// Thermistor PWM pinout. First number in a pair is the PWM pin (output), second thermistor PIN (analog input)
// IMPORTANT: there is no compiler check for pinout size, so be careful.
#define APB_PWM_OUTPUTS_PWM_PINOUT {APB_PWM_0_PIN,APB_THERMISTOR_0_PIN, Heater}, \
                                   {APB_PWM_1_PIN,APB_THERMISTOR_1_PIN, Heater}, \
                                   {APB_PWM_2_PIN,-1, Output}, \
                                   {APB_PWM_3_PIN,-1, Output}, \
                                   {APB_PWM_4_PIN,-1, Output}, \
                                   {APB_PWM_5_PIN,-1, Output}
#define APB_PWM_OUTPUTS_SIZE 6
#define APB_PWM_OUTPUTS_TEMP_SENSORS 2
#ifndef APB_CONFIGURATION_H
#define APB_CONFIGURATION_H

#define __APB__SIM 0

#define __APB__SHT30 1
#define __APB__BME280 2
#define __APB__DHT11 3

#define __APB__THERMISTOR 1

#if __has_include ("configuration_custom.h")
#include "configuration_custom.h"
#endif

// Defaults if configuration_custom.h is not found
#ifndef APB_MAX_STATIONS
#define APB_MAX_STATIONS 5
#endif

#ifndef APB_HEATERS_SIZE
#define APB_HEATERS_SIZE 3
#endif

#ifndef APB_AMBIENT_UPDATE_INTERVAL_SECONDS
#define APB_AMBIENT_UPDATE_INTERVAL_SECONDS 5
#endif

#ifndef APB_HEATER_UPDATE_INTERVAL_SECONDS
#define APB_HEATER_UPDATE_INTERVAL_SECONDS 5
#endif


#ifndef APB_AMBIENT_TEMPERATURE_SENSOR
#define APB_AMBIENT_TEMPERATURE_SENSOR __APB__SIM
#endif

#ifndef APB_HEATER_TEMPERATURE_SENSOR
#define APB_HEATER_TEMPERATURE_SENSOR __APB__SIM
#endif

#ifndef APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR_RESISTOR
#define APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR_RESISTOR 10'000
#endif


// replace in configuration_override.h as needed, be sure that pinout values are unique
#ifndef APB_HEATERS_PWM_PINOUT
// Thermistor PWM pinout. First number in a pair is the PWM pin (output), second thermistor PIN (analog input)
// IMPORTANT: there is no compiler check for pinout size, so be careful.
#define APB_HEATERS_PWM_PINOUT {0,1}, {2,3}, {4,5}
#endif

// Derivative constants, DO NOT change/redefine


#if APB_HEATER_TEMPERATURE_SENSOR == __APB__SIM
#define APB_HEATER_TEMPERATURE_SENSOR_SIM
#elif APB_HEATER_TEMPERATURE_SENSOR == __APB__THERMISTOR
#define APB_HEATER_TEMPERATURE_SENSOR_THERMISTOR
#else
#error APB_HEATER_TEMPERATURE_SENSOR undefined. Please change configuration_custom.h
#endif

#if APB_AMBIENT_TEMPERATURE_SENSOR == __APB__SIM
#define APB_AMBIENT_TEMPERATURE_SENSOR_SIM
#elif APB_AMBIENT_TEMPERATURE_SENSOR == __APB__SHT30
#define APB_AMBIENT_TEMPERATURE_SENSOR_SHT30
#elif APB_AMBIENT_TEMPERATURE_SENSOR == __APB__BME280
#define APB_AMBIENT_TEMPERATURE_SENSOR_BME280
#elif APB_AMBIENT_TEMPERATURE_SENSOR == __APB__DHT11
#define APB_AMBIENT_TEMPERATURE_SENSOR_DHT11
#else
#error APB_AMBIENT_TEMPERATURE_SENSOR undefined. Please change configuration_custom.h
#endif

#endif

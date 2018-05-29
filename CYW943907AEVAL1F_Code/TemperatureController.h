#ifndef TEMPERATURECONTROLLER_H
#define TEMPERATURECONTROLLER_H

#include "wiced.h"
#include "TemperatureMeter.h"
// TODO include the thing that has information about how many people are in the building

// these are in celcius
#define LOWER_TEMPERATURE_BOUNDARY 20
#define UPPER_TEMPERATURE_BOUNDARY 21.11111111

// pin 1 on the J6 header
#define HEATER_PIN WICED_GPIO_17
// pin 2 on the J6 header 
#define AC_PIN WICED_GPIO_18


#define NO_CONTROL 0
#define AC 1
#define HEATER 2

static volatile int people = 0; 
static int controlState = NO_CONTROL;

void turnOffAC() {
    wiced_gpio_output_high(AC_PIN);
}

void turnOffHeater() {
	wiced_gpio_output_high(HEATER_PIN);
}

void turnOnHeater() {
	turnOffAC();
	wiced_gpio_output_low(HEATER_PIN);
}

void turnOnAC() {
	turnOffHeater();
	wiced_gpio_output_low(AC_PIN);
}

void checkIfControlIsNeeded() {
	if(temperature < LOWER_TEMPERATURE_BOUNDARY) {
		controlState = HEATER;
		turnOnHeater();
	} else if(temperature > UPPER_TEMPERATURE_BOUNDARY) {
		controlState = AC;
		turnOnAC();
	}
}

void checkIfStillNeedAC() {
	if(temperature < LOWER_TEMPERATURE_BOUNDARY) {
		controlState = NO_CONTROL;
		turnOffAC();
	}
}

void checkIfStillNeedHeater() {
	if(temperature > UPPER_TEMPERATURE_BOUNDARY) {
		turnOffHeater();
		controlState = NO_CONTROL;
	}
}

void controlTemperature() {
	if(people < 1) {
		controlState = NO_CONTROL;
	}
	switch(controlState) {
		case NO_CONTROL:
			checkIfControlIsNeeded();
			break;
		case AC:
			checkIfStillNeedAC();
			break;
		case HEATER:
			checkIfStillNeedHeater();
			break;
	}
}

#endif

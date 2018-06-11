/* This code is for the ATTiny85. It listens to two accelerometers that are
 * build into a floor mat that people walk on to enter or exit a building.
 * It uses the two accelerometers to detect when people enter or leave and 
 * keeps track of how many people are in the building or room. It communicates
 * with an ESP8266-01 over I2C as a slave how many people are in the room or
 * building. That information is used to control the heating and cooling of 
 * the room or building. This code was designed to use two ADXL337.
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "TinyWire.h"

#define I2C_ADDR 0x0A
#define INNER PB3
#define OUTER PB4
// middle should be about 337 (1.65v) for a 3.3v supply ADC
#define MIDDLE_VAL 300
#define ADC_THRESHOLD 100

// states
#define NOTHING 0
#define GOT_INNER 1
#define GOT_OUTER 2
int state = NOTHING;

// WAIT_TIME_THRESHOLD is the amount of milliseconds to determine if somebody 
// is entering or exiting the room or building.
#define WAIT_TIME_THRESHOLD 1000
unsigned long changeTime = 0;

volatile char people = 0;

/* This ISR is triggered when the master requests data from this device.
 * It will send a single byte representing the number of people in the room.
 */
void i2cISR() {
	TinyWire.send(people);
}

// get the magnitude of the response from zero point of ADC
int sample(int pin) {
	return abs(analogRead(pin)-MIDDLE_VAL);
}

void setup() {
	TinyWire.begin(I2C_ADDR);
	TinyWire.onRequest(i2cISR);
	MCUCR |= 1<<6;
	pinMode(INNER, INPUT);
	pinMode(OUTER, INPUT);
}

/* Poll both accelerometers to detect if somebody may be entering or exiting.
 * This is the default state of the state machine.
 */
void waitForSomeone() {
	if(sample(INNER) > ADC_THRESHOLD) {
		changeTime = millis();
		state = GOT_INNER;
	} else if(sample(OUTER) > ADC_THRESHOLD) {
		changeTime = millis();
		state = GOT_OUTER;
	}
}

/* Wait for one of the accelerometers to trigger. This is called after the
 * other accelerometer has registered a significant change in its value, which
 * indicates that someone has stepped on the mat and may be entering or exiting
 * the room or building.
 */
void waitFor(int pin, bool increment) {
	while(changeTime-millis() < WAIT_TIME_THRESHOLD) {
		if(sample(pin) > ADC_THRESHOLD) {
			if(increment && people == 127) { // too many people are in the room
				break;
			}
			people = increment ? people+1 : people-1;
			if(people < 0) {
				people = 0;
			}
			break;
		}
	}
	changeTime = millis();
	state = NOTHING;
}

/* A state machine that has 3 states. 
 * The NOTHING state waits until one of the accelerometers has registered 
 * that somebody has stepped on the mat. The GOT_INNER state means that 
 * somebody may be exiting the room, therefore the other accelerometer should
 * be listened. The GOT_OUTER state is the exact opposite of the GOT_INNER 
 * state. Somebody is likely entering, therefore the inner accelerometer 
 * should be listened to.
 */
void loop() {
	switch(state) {
		case NOTHING:
			waitForSomeone();
			break;
		case GOT_INNER:
			// somebody is entering
			waitFor(OUTER, true);
			break;
		case GOT_OUTER:
			// somebody is leaving
			waitFor(INNER, false);
			break;
	}
}

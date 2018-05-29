#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "TinyWire.h"

#define mAddr 0x0A
#define INNER PB3
#define OUTER PB4
// middle should be about 337 (1.65v) for a 3.3v supply ADC
#define MIDDLE_VAL 337
#define ADC_THRESHOLD 100

// states
#define NOTHING 0
#define GOT_INNER 1
#define GOT_OUTER 2
int state = NOTHING;

#define WAIT_TIME_THRESHOLD 1000
unsigned long changeTime = 0;

volatile char people = 0;

void i2cISR() {
	TinyWire.send(people);
}

// get response from zero point of ADC
int sample(int pin) {
	return abs(analogRead(pin)-MIDDLE_VAL);
}

void setup() {
	TinyWire.begin(mAddr);
    TinyWire.onRequest(i2cISR);
    pinMode(INNER, INPUT);
    pinMode(OUTER, INPUT);
}

void waitForSomeone() {
	if(sample(INNER) > ADC_THRESHOLD) {
		changeTime = millis();
		state = GOT_INNER;
	} else if(sample(OUTER) > ADC_THRESHOLD) {
		changeTime = millis();
		state = GOT_OUTER;
	}
}

void waitFor(int pin, bool increment) {
	while(changeTime-millis() < WAIT_TIME_THRESHOLD) {
		if(sample(pin) > ADC_THRESHOLD) {
			people = increment ? people+1 : people-1;
			changeTime = millis();
			state = NOTHING;
			break;
		}
	}
}

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
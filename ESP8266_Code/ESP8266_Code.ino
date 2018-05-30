#include <ESP8266WiFi.h>
#include "brzo_i2c.h"

#define SDA_PIN 0
#define SCL_PIN 2
#define SCL_SPEED 200
// in microseconds
#define SCL_STRETCH_TIMEOUT 50000

#define ATTINY85_ADDR 0x0A

// TODO choose these things
#define ssid "Not4u"
#define PASSWORD "DERPDERP"
#define IPADDR 192,168,141,1
#define PORT 12345

WiFiClient wifiConn; 
IPAddress cypressBoard(IPADDR);

static volatile uint8_t numPeople = 0;
static uint8_t oldPeople = 0;

void pollForPeople() {
	uint8_t numPeopleTemp = 0;
	brzo_i2c_start_transaction(ATTINY85_ADDR, SCL_SPEED);
	brzo_i2c_read(&numPeopleTemp, 1, true); // TODO verify that repeated_start should be true 
	numPeople = numPeopleTemp;
}

void sendInfoIfChanged() {
	if(oldPeople != numPeople) {
		char peopleStr[4];
		sprintf(peopleStr, "%d", numPeople);
		wifiConn.println(peopleStr);
		oldPeople = numPeople;
	}
}

void setup() {
	Serial.begin(115200);
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	delay(1000);
	brzo_i2c_setup(SDA_PIN, SCL_PIN, SCL_STRETCH_TIMEOUT);
	Serial.println("Setup done");
	WiFi.begin(ssid, PASSWORD);
	while(WiFi.status() != WL_CONNECTED) {
		Serial.println("Could not connect. Trying again in 1 second");
		delay(1000);
	}
	Serial.println(WiFi.localIP());
	while(!wifiConn.connect(cypressBoard, PORT)) {
      // everything is setup, not sure what we should do here
		Serial.println("Could not connect to Cypress board. Trying again in 1 second.");
		delay(1000);
	}
	Serial.println("Connections have been made");
}

void loop() {
	delay(1000);
	pollForPeople();
	sendInfoIfChanged();
}

/* This code is for the ESP8266-01 to listen to an ATTiny85 as to how many 
 * people are in a room or building. The objective is to forward this info
 * to a cypress board that is controlling the AC or heater of a building or
 * room. 
 *
 * It communicates with the ATTiny85 as the master on the I2C.
 */
#include <ESP8266WiFi.h>
#include <Wire.h>

// #define DEBUG

#define SDA_PIN 0
#define SCL_PIN 2
#define SCL_SPEED 200
// in microseconds
#define SCL_STRETCH_TIMEOUT 50000

#define ATTINY85_ADDR 0x0A

// TODO change these things
#define ssid "Not4u"
#define PASSWORD "DERPDERP"
#define IPADDR 192,168,43,125
#define PORT 12345

WiFiClient wifiConn; 
IPAddress cypressBoard(IPADDR);

// numPeople is the latest information from the ATTiny85
uint8_t numPeople = 0;
// oldPeople is the last transmission sent to the cypress board
uint8_t oldPeople = 0;

/* Poll the ATTiny85 for how many people are in the room
 * This function sets numPeople based on what is read from the ATTiny85
 */
void pollForPeople() {
	Serial.println("Polling for people");
	uint8_t numPeopleTemp = 0;
	Wire.requestFrom(ATTINY85_ADDR, 1); // requests 1 byte
	while(Wire.available()) {
		numPeopleTemp = Wire.read();
		#ifdef DEBUG
			Serial.printf("Got %d people\n", numPeopleTemp);
		#endif
	}
	numPeople = numPeopleTemp;
}

/* If the latest information from the ATTiny85 is different than what was last 
 * sent to the cypress board, inform the cypress board.
 */
void sendInfoIfChanged() {
	if(oldPeople != numPeople) {
		char peopleStr[4];
		sprintf(peopleStr, "%d", numPeople);
		#ifdef DEBUG
			Serial.print("Read %s from ATTiny85.\n", peopleStr);
		#endif
		wifiConn.println(peopleStr);
		oldPeople = numPeople;
	}
}

void setup() {
	Serial.begin(115200);
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	delay(1000);
	Wire.begin(SDA_PIN, SCL_PIN);
	Serial.println("Setup done");
	WiFi.begin(ssid, PASSWORD);
	while(WiFi.status() != WL_CONNECTED) {
		Serial.println("Could not connect. Trying again in 1 second");
		delay(1000);
	}
	#ifdef DEBUG
		Serial.println(WiFi.localIP());
	#endif
	while(!wifiConn.connect(cypressBoard, PORT)) {
		#ifdef DEBUG
			Serial.println("Could not connect to Cypress board. Trying again in 1 second.");
		#endif
		delay(1000);
	}
	#ifdef DEBUG
		Serial.println("Connections have been made");
	#endif
}

/* Continuously poll for data and send it if it has changed
 * This can definitely be made more power efficient.
 */
void loop() {
	delay(1000);
	pollForPeople();
	sendInfoIfChanged();
}

#include <list>
#include "neopixel.h"

TCPClient client;
bool kevin;
bool elin;

// RGBA
uint32_t colorRed = 0xFF000050;
uint32_t colorGreen =  0X00FF0050;

#define PIXEL_PIN D2
#define PIXEL_COUNT 2
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

bool connect(byte ipAddress[]) {
	if (client.connect(ipAddress, 80)) {
		client.println("GET /update_clients.asp HTTP/1.0");
		client.println("HOST: router");
		client.println("Content-Length: 0");
		client.println();
		return true;
	}
	return false;
}

void populateBuffer(byte ipAddress[], String *readBuffer) {
	bool connected = connect(ipAddress);
	long cnt = 0;
	while (connected) {
		if (client.available()) {
			String character = String((char) client.read());
			readBuffer->concat(character);
		} else {
			cnt++;
		}

		if (!client.connected() || cnt >= 2000) {
			client.stop();
			connected = false;
		}
	}
}

void parseMacs(String readBuffer, String key1, String key2, std::list<String> *macs) {
	long keyIndex1 = readBuffer.indexOf(key1);
	long keyIndex2 = readBuffer.indexOf(key2);

	String startKey = String("[\"");
	String endKey = String("\",");

	long startIndex = keyIndex1;
	long endIndex;

	while (true) {
		startIndex = readBuffer.indexOf(startKey, startIndex);
		endIndex = readBuffer.indexOf(endKey, startIndex);
		if (startIndex < keyIndex2 && endIndex != -1) {
			macs->push_back(readBuffer.substring(startIndex+2, endIndex));
			startIndex = endIndex;
		} else {
			break;
		}
	}
}

void addMacs(byte ipAddress[], std::list<String> *macs) {
	String key1 = String("wlList_2g: [");
	String key2 = String("wlList_5g: [");
	String key3 = String("wlList_5g_2: [");
	String readBuffer;
	populateBuffer(ipAddress, &readBuffer);
	parseMacs(readBuffer, key1, key2, macs);
	parseMacs(readBuffer, key2, key3, macs);
}

bool containsMac(String mac, std::list<String> *macs) {
	for (std::list<String>::iterator it=macs->begin(); it != macs->end(); ++it) {
		if (it->compareTo(mac) == 0) {
			return true;
		}
	}
	return false;
}

void updateState(){
	std::list<String> macs;
	byte ipAddress1[] = {192, 168, 1, 253};
	byte ipAddress2[] = {192, 168, 1, 254};
	addMacs(ipAddress1, &macs);
	addMacs(ipAddress2, &macs);

	kevin = containsMac("04:4B:ED:7D:7F:72", &macs);
	elin = containsMac("24:F0:94:71:1B:03", &macs);

/*	Serial.print("Kevin ");
        Serial.println(kevin);
        Serial.print("Elin ");
        Serial.println(elin);
        Serial.println("");
        for (std::list<String>::iterator it=macs.begin(); it != macs.end(); ++it) {
                Serial.println(*it);
        }
 */
	macs.clear();
}

void setup() {
	Serial.begin(9600);

	for(int i = 16; i > 0; i--) {
		Serial.println("Starting in " + String(i,DEC));
		delay(1000);
	}

	strip.begin();
	strip.show();
}

void display(int diodNumber, bool home) {
	uint32_t color = home ? colorGreen : colorRed;
	strip.setColorDimmed(diodNumber, color>>24&255, color>>16&255, color>>8&2550, color&255),
	strip.show();
}

void updateOutput() {
	display(0, kevin);
	display(1, elin);
}

void loop() {
	updateState();
	updateOutput();
	delay(30000);
}

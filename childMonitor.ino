#include "neopixel.h"
#include <list>

/**
 * A define AUTHENTICATION needs to be provided.
 * It should be a Base64 encoded String containing username:password
 * Exactly as a Basic Authentication value.
 * Added in a separate file that isn't added to git for security
 * reasons.
 *
 * Example:
 * #define AUTHENTICATION "YWRtaW46cGFzc3dvcmQK"
 */
#include "authentication.h"

// RGBA
#define WHITE 0xFFFFFF50
#define RED 0xFF000050
#define GREEN 0x00FF0050
#define YELLOW 0xFFFF0050

#define MOBILE_COUNT 2

struct MOBILE {
  String mac;              // The MAC address of the phone
  bool present;            // true if phone is detected on the network
  uint8_t hourLastPresent; // Hour when last went to Green
  uint32_t color;          // Actual color R,G,B,intensity
  int count;               // Used by the logic to determine state change.
};

struct MOBILE mobiles[MOBILE_COUNT];

long errorCounter;

TCPClient client;

#define SESSION_COOKIE "asus_token"

#define PIXEL_PIN D2
#define PIXEL_COUNT 2
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

void setup() {
  Serial.begin(9600);

  Time.zone(1);

  strip.begin();
  addMobile("04:4B:ED:7D:7F:72", 0); // Kevin
  addMobile("24:F0:94:71:1B:03", 1); // Elin
  strip.show();

  errorCounter = 0;
  for (int i = 20; i > 0; i--) {
    Serial.println("Starting in " + String(i, DEC));
    delay(1000);
  }
}

void loop() {
  updateMACState();
  updateDiodState();
  delay(30000);
}

void addMobile(String mac, int position) {
  struct MOBILE *mobile = &mobiles[position];
  mobile->mac = mac;
  mobile->color = WHITE;
  mobile->count = 0;
  mobile->hourLastPresent = 12;
  mobile->present = false;

  display(position, mobile->color);
}

String login(byte ipAddress[]) {
  if (client.connect(ipAddress, 80)) {
    client.println("POST /login.cgi HTTP/1.1");
    addCSRFHeaders(client, ipAddress);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(20 + sizeof(AUTHENTICATION));
    client.println();
    client.print("login_authorization=");
    client.println(AUTHENTICATION);

    String readBuffer;
    populateBuffer(ipAddress, &readBuffer);
    return extractSessionCookie(&readBuffer);
  } else {
    Serial.print("ERROR connecting to ");
    Serial.println(ipAddressToString(ipAddress));
    return "";
  }
}

void addCSRFHeaders(TCPClient client, byte ipAddress[]) {
  String ipString = ipAddressToString(ipAddress);
  client.print("Host: ");
  client.println(ipString);
  client.print("Origin: http://");
  client.println(ipString);
  client.print("Referer: http://");
  client.print(ipString);
  client.println("/Main_Login.asp");
}

String extractSessionCookie(String *readBuffer) {
  long startIndex = readBuffer->indexOf(SESSION_COOKIE);
  long endIndex = readBuffer->indexOf(";", startIndex);

  if (startIndex != -1 && endIndex != -1) {
    return readBuffer->substring(startIndex + 11, endIndex);
  } else {
    return "";
  }
}

void getConnectedMacs(byte ipAddress[], String sessionCookie,
                      String *readBuffer) {
  if (client.connect(ipAddress, 80)) {
    client.println("GET /update_clients.asp HTTP/1.0");
    addCSRFHeaders(client, ipAddress);
    client.println("Content-Length: 0");
    client.print("Cookie: ");
    client.print(SESSION_COOKIE);
    client.print("=");
    client.println(sessionCookie);
    client.println();
    populateBuffer(ipAddress, readBuffer);
  } else {
    Serial.println("ERROR");
  }
}

void populateBuffer(byte ipAddress[], String *readBuffer) {
  long cnt = 0;
  bool connected = true;
  while (connected) {
    if (client.available()) {
      String character = String((char)client.read());
      readBuffer->concat(character);
    } else {
      cnt++;
    }

    if (!client.connected() || cnt >= 20000) {
      client.stop();
      connected = false;
    }
  }
}

void parseMacs(String readBuffer, String key1, String key2,
               std::list<String> *macs) {
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
      macs->push_back(readBuffer.substring(startIndex + 2, endIndex));
      startIndex = endIndex;
    } else {
      break;
    }
  }
}

bool addMacs(byte ipAddress[], std::list<String> *macs) {
  String key1 = String("wlList_2g: [");
  String key2 = String("wlList_5g: [");
  String key3 = String("wlList_5g_2: [");

  String sessionCookie = login(ipAddress);
  if (sessionCookie.length() == 0) {
    return false;
  }
  String readBuffer;
  getConnectedMacs(ipAddress, sessionCookie, &readBuffer);
  parseMacs(readBuffer, key1, key2, macs);
  parseMacs(readBuffer, key2, key3, macs);
  return true;
}

bool containsMac(String mac, std::list<String> *macs) {
  for (std::list<String>::iterator it = macs->begin(); it != macs->end();
       ++it) {
    if (it->compareTo(mac) == 0) {
      return true;
    }
  }
  return false;
}

void updateMACState() {
  std::list<String> macs;
  byte ipAddress1[] = {192, 168, 1, 253};
  byte ipAddress2[] = {192, 168, 1, 254};
  bool success = addMacs(ipAddress1, &macs);
  if (!success) {
    errorCounter++;
    Serial.print("Error ");
    Serial.println(ipAddressToString(ipAddress1));
    return;
  }
  success = addMacs(ipAddress2, &macs);
  if (!success) {
    errorCounter++;
    Serial.print("Error ");
    Serial.println(ipAddressToString(ipAddress2));
    return;
  }

  for (int i = 0; i < MOBILE_COUNT; i++) {
    bool present = containsMac(mobiles[i].mac, &macs);
    // Record hour if going from !present -> present.
    if (!mobiles[i].present && present) {
      mobiles[i].hourLastPresent = Time.hour();
    }
    mobiles[i].present = present;
  }
  //	Serial.print("Kevin ");
  //    Serial.println(kevin);
  //    Serial.print("Elin ");
  //    Serial.println(elin);
  //    for (std::list<String>::iterator it=macs.begin(); it != macs.end();
  //    ++it) {
  //		Serial.println(*it);
  //    }
  //	Serial.println("");

  macs.clear();
  errorCounter = 0;
  Serial.println("State updated");
}

void updateDiodState() {
  for (int i = 0; i < MOBILE_COUNT; i++) {
    struct MOBILE *mobile = &mobiles[i];
    if (errorCounter > 10) {
      // More than 10 errors in a row when trying to update MAC presens.
      mobile->color = YELLOW;
      mobile->count = 0;
    } else if (mobile->present) {
      // Always set Green if present.
      mobile->color = GREEN;
      mobile->count = 0;
    } else if (mobile->hourLastPresent > 18 || mobile->hourLastPresent < 7) {
      // If has been present (Red -> Green) between 22 and 6 then count up.
      if (mobile->count > 5) {
        // 10 times non present, but has been present between 22 and 6.
        // Display as Yellow since this might be a case of out of battery.
        mobile->color = YELLOW;
      } else {
        // Not yet 10 times in a row. Increase counter.
        mobile->count++;
      }
    } else {
      //  Not present and time has not been home between 22 and 6.
      mobile->color = RED;
    }

    // Display the color
    display(i, mobile->color);
  }
}

void display(int diodNumber, uint32_t color) {
  strip.setColorDimmed(diodNumber, color >> 24 & 255, color >> 16 & 255,
                       color >> 8 & 2550, color & 255);
  strip.show();
}

String ipAddressToString(byte ipAddress[]) {
  String result = "";
  for (int i = 0; i < 4; i++) {
    char str[16];
    sprintf(str, "%d", ipAddress[i]);
    result.concat(str);
    if (i < 3) {
      result.concat(".");
    }
  }
  return result;
}

/*
   Irisdown Countdown Timer : LED Display Remote for Olimex ESP32-GATEWAY/ESP32-POE

   Listens for UDP packets on port 61003. Shows time on
   Adafruit clock 4-digit 7-segment display.

   created 10/01/2019
   by David Shepherd

   http://http://www.irisdown.co.uk/countdowntimer.html

   Libraries required:
     Adafruit GFX Library v1.5.7
     Adafruit LED Backpack Library v1.1.6

   Board required:
     esp32 by Espressive Systems Board v1.0.4 (v1.0.0 has seriously unreliable I2C)

   For ESP32-POE:
     Connections:
       ESP-32          LED Backpack
       +5V             VCC
       GND             GND
       GPIO13/I2C-SDA  SDA
       GPIO16/I2C-SCL  SCL
     Settings:
       #define ETH_PHY_POWER 12

*/

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12 // ESP32-GATEWAY 5 and ESP32-POE 12 !!!
#define DISPLAY_ADDRESS 0x70
#define EEPROM_SIZE 1

#include <WiFi.h>
#include "AsyncUDP.h"
#include <Wire.h>
// ESP-32 Gateway: use pin 32 for SDA and define in Adafruit_LEDBackpack.cpp
// line 206: Wire.begin(32,16); as ethernet uses std pin 17 !!!
// ESP-32 POE: no configuration change required
#include "Adafruit_LEDBackpack.h"
#include <EEPROM.h>


// network vars
bool ethConnected = false;
bool goodHeader = true;

AsyncUDP udp;
// Porty 61003
//36700

const unsigned int udpPort = 36700;
unsigned long udpLastPacketMillis = 0;
const unsigned int udpTimeout = 3000;
unsigned long udpTime = 0;      // Time remaining in seconds.
bool udpTimeNegative = false;   // If true, udpTime is time since 00:00.
int udpId = 0;                  // id of the master unit.

// LED display vars
int ledBrightness = 15; // maxmimum brightness
Adafruit_7segment ledDisplay = Adafruit_7segment();

// System vars
int id = 0;                       // id of this unit (to be read from EEPROM)
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
const int intervalMillis = 100;
bool modeChanged = false;
const int initTimeout = 5000;
int initCounter = 0;
unsigned long initCurrentMillis = 0;
unsigned long initPreviousMillis = 0;
const int initIntervalMillis = 500;
const int menuTimeout = 4000;
unsigned long menuMillis = 0;
const char* ssid = "cma";
const char* password =  "@@cmacma@@";


// Menu button.
const int buttonPin = 0;
int buttonState = HIGH;
int lastButtonState = HIGH;

// opMode will change to match current status of the device:
enum opMode {
  Menu = -1,  // Show 'id: x' and increment on button press. Clear after menuTimeout.
  Init = 0,   // Chase digits and display id for a few seconds.
  No_Eth = 1, // Eth down, show 'conn'.
  No_Rx = 2,  // No packet with matching id received in the last udpTimeout ms, show '--:--'.
  Normal = 3, // Normal operation, show clock.
};
opMode currentMode = Init;

void WiFiEvent(WiFiEvent_t event) { // This event handler updates the connection status of ETH.
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");

      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("WIFI Connected");
      ethConnected = true;
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(WiFi.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(WiFi.localIP());
      WiFi.setHostname("esp32-ethernet");

      ethConnected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      ethConnected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      ethConnected = false;
      break;
  }
}

void udpPacketEvent(AsyncUDPPacket packet) { // This event handler receives every incoming udp packet
  // Two checks to validate packet structure:
  // 1) Confirm packet length of 20.
  //Serial.println("PACKET RECEIVED!");
  if (packet.length() == 3) { // Packet length has to be 3 bytes!

    // 2) Confirm packet header of 'IDCT:'
    //const char header[] = {'I','D','C','T',':','\0'}; //Not this protocol
    goodHeader = true;
    //Serial.print("Packet length: ");
    //Serial.println(packet.length());
    


    

    if (goodHeader == true) {
      //Serial.println("Packet OK! Yummy!");
      // Get id which is a single hex character '0'-'9','A'-'F'
      /*
      const char hexchar = packet.data()[12];
      udpId = (hexchar >= 'A') ? (hexchar - 'A' + 10) : (hexchar - '0'); //char must be uppercase
      //get packet time in seconds
      const char packetTime[] = {packet.data()[6], packet.data()[7], packet.data()[8], packet.data()[9], packet.data()[10], packet.data()[11], '\0'};
      udpTime = atoi(packetTime);
      //get packet time sign =/-
      udpTimeNegative = false;
      if (packet.data()[5] == '-') {
        udpTimeNegative = true;
      }
      // If packet id matches our id we can reset udpLastPacketMillis.
      if (udpId == id) {
        udpLastPacketMillis = millis();
      }
      */

      const char hexchar = packet.data()[3];
      //String hexchar = (const char*)packet.data();
      //const char packetTime[] = {packet.data()[6], packet.data()[7], packet.data()[8], packet.data()[9], packet.data()[10], packet.data()[11], packet.data()[12], packet.data()[13], packet.data()[14], packet.data()[15], packet.data()[16], packet.data()[17], packet.data()[18], packet.data()[19], packet.data()[20],   '\0'};
      const char packetTime[] = {packet.data()[6], packet.data()[7], packet.data()[8], '\0'};
      
      Serial.print("Byte 1 Minutes: ");
      int bcd1 = (packet.data()[0], HEX);
      Serial.print(packet.data()[0], HEX);
      Serial.println("");
      Serial.print("Byte 2 Seconds: ");
      Serial.print(packet.data()[1], HEX);
      Serial.println("");
      Serial.print("Byte 3 COLOR: ");
      
      Serial.print(packet.data()[2], HEX);
      Serial.println("----");
      Serial.println("");

     
      

      
    }
  } else {
    goodHeader = false;
    Serial.println("Packet is not 3 bytes - not for me!");

  }
  //debug stuff
  /*
    Serial.print("UDP Packet Type: ");
    Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
    Serial.print(", From: ");
    Serial.print(packet.remoteIP());
    Serial.print(":");
    Serial.print(packet.remotePort());
    Serial.print(", To: ");
    Serial.print(packet.localIP());
    Serial.print(":");
    Serial.print(packet.localPort());
    Serial.print(", Length: ");
    Serial.print(packet.length());
    Serial.print(", Data: ");
    Serial.write(packet.data(), packet.length);
    Serial.println();
  */
}

void setup() {
  Serial.begin(115200);

  // Init display:
  ledDisplay.begin(DISPLAY_ADDRESS);
  ledDisplay.setBrightness(ledBrightness);
  // Read id from EEPROM, and set to 0 if out of bounds.
  EEPROM.begin(EEPROM_SIZE);
  id = EEPROM.read(0);
  if (id < 0 || id > 15) {
    id = 0;
    EEPROM.write(0, id);
    EEPROM.commit();
  }
  Serial.println((String)"id: " + id);
  // Init eth with a random link-local address:
  randomSeed(analogRead(0));
  IPAddress ip(169, 254, random(1, 255), random(1, 255));
  IPAddress gateway(169, 254, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress dns(169, 254, 1, 1);
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  //WiFi.config(ip, gateway, subnet, dns);
  // Start udp listener.
  if (udp.listen(udpPort)) {
    Serial.print("UDP Listening on Port: ");
    Serial.println(udpPort);
    udp.onPacket(udpPacketEvent);
  }
  pinMode(buttonPin, INPUT); // get button ready for input
}

void loop() {
  // Execute every 100ms.
  currentMillis = millis();
  if (currentMillis - previousMillis >= intervalMillis) {
    previousMillis = currentMillis;

    // Update mode?
    modeChanged = false;
    opMode newMode = GetMode();
    if (newMode != currentMode) {
      modeChanged = true;
      // Mode change!
      Serial.print("Mode change: ");
      Serial.print(currentMode);
      Serial.print(" -> ");
      Serial.println(newMode);
    }

    // Process mode
    switch (newMode) {
      case Menu :
        if (buttonPressed()) {
          // increase unit id
          incId();
          // and stay in menu mode for a bit longer
          menuMillis = millis();
        }
        showMenu();
        break;
      case Init :
        showInit();
        break;
      case No_Eth :
        showNoEth();
        break;
      case No_Rx :
        showNoRx();
        break;
      case Normal :
        showClock();
        break;
    }
    currentMode = newMode;
  }
}

opMode GetMode() {
  if (millis() < initTimeout) {
    return Init;  // Enter init at startup.
  }
  if ((millis() - menuMillis) < menuTimeout) {    // Stay in menu mode until menuTimeout has elapsed.
    return Menu;
  } else {
    if (buttonPressed()) {                        // Enter menu mode.
      menuMillis = millis();
      return Menu;
    }
  }
  if (!ethConnected) {
    return No_Eth;  // No ethernet.
  }
  if (udpLastPacketMillis == 0) {
    return No_Rx;  // No id matching packet received yet.
  }
  if ((millis() - udpLastPacketMillis) > udpTimeout) {
    return No_Rx;  // Last id matching packet was too long ago.
  }
  return Normal;
}

void showMenu() {
  // Draw id:id
  ledDisplay.print(id, DEC);
  ledDisplay.writeDigitRaw(0, 4); // i...
  ledDisplay.writeDigitRaw(1, 94); // .d..
  ledDisplay.drawColon(true);
  ledDisplay.writeDisplay();
}

void showInit() {
  // Quick animation to check all digits then show id (from menu).
  initCurrentMillis = millis();
  if (initCurrentMillis - initPreviousMillis >= initIntervalMillis) {
    initPreviousMillis = initCurrentMillis;

    if (initCounter > 4) {
      showMenu();
      return;
    }
    ledDisplay.writeDigitRaw(0, 0);
    ledDisplay.writeDigitRaw(1, 0);
    ledDisplay.writeDigitRaw(3, 0);
    ledDisplay.writeDigitRaw(4, 0);
    ledDisplay.writeDigitRaw(initCounter, 127);
    ledDisplay.writeDisplay();
    initCounter += 1;
    if (initCounter == 2) {
      initCounter++;  // this jumps over the colon separating minutes and seconds
    }
  }
}

void showNoEth() {
  // Draw 'conn' to indicate we have a connection problem.
  ledDisplay.writeDigitRaw(0, 88);
  ledDisplay.writeDigitRaw(1, 92);
  ledDisplay.writeDigitRaw(3, 84);
  ledDisplay.writeDigitRaw(4, 84);
  ledDisplay.drawColon(false);
  ledDisplay.writeDisplay();
}

void showNoRx() {
  // Draw '--:--' to indicate we are in a ready state, but no packets with that id are being detected.
  ledDisplay.writeDigitRaw(0, 64);
  ledDisplay.writeDigitRaw(1, 64);
  ledDisplay.writeDigitRaw(3, 64);
  ledDisplay.writeDigitRaw(4, 64);
  ledDisplay.drawColon(true);
  ledDisplay.writeDisplay();
}

void showClock() {
  // Convert to base 60 & just show hours/mins if over 99m59s.
  int s = udpTime % 60;
  int m = floor(udpTime / 60);
  unsigned long displayValue = 0;
  if (udpTime <= 5999) {
    displayValue = (m * 100) + s;
    Serial.println(displayValue);
  } else {
    // Cant show the time in full so
    // just show the hours and minutes.
    int h = floor(m / 60);
    m = m % 60;
    displayValue = (h * 100) + m;
    Serial.println(displayValue);

  }
  // Send the time value to the display.
  ledDisplay.print(displayValue, DEC);
  ledDisplay.drawColon(true);
  // Add leading zeroes at positions 1 & 3 '-0:0-' if required.
  if (m == 0) {
    ledDisplay.writeDigitNum(1, 0);
    if (s < 10) {
      ledDisplay.writeDigitNum(3, 0);
    }
  }
  // If we are in overtime, show a -ve sign at position 0 while we can (<10mins).
  if (udpTimeNegative && m < 10) {
    ledDisplay.writeDigitRaw(0, 64);
  }
  // Finally push out to the display.
  ledDisplay.writeDisplay();
}

bool buttonPressed() {
  bool reply = false;
  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    reply = true;
  }
  lastButtonState = buttonState;
  return reply;
}

void incId() {
  id += 1;
  if (id > 15) {
    id = 0;
  }
  Serial.print("NEW ID: ");
  Serial.println(id);
  EEPROM.write(0, id);
  EEPROM.commit();
}

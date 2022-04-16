/*

   Display for UDP Time protocol used by Interspace Industries CDEther Receiver, Irisdown Countdown Timer 2.0, Neodarque Stage Timer 2 and Clock-8001
   https://www.interspaceind.com/shop-our-products/product/112-cdether-receiver/category_pathway-37.html
   https://www.irisdown.co.uk/countdowntimer.html
   https://www.neodarque.com/StageTimer
   https://gitlab.com/Depili/clock-8001
   
   I am using that display with clock-8001 that sends the UDP time on two ports: 36700 and 36701

  
   Listens for UDP packets on port 36700. Shows time on:
   1) Serial console
   
   Parts of code by David Shepherd

   Libraries required:

   Board required:
     esp32 by Espressive Systems Board v1.0.4 (v1.0.0 has seriously unreliable I2C)
  
*/

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12 // ESP32-GATEWAY 5 and ESP32-POE 12 !!!
#define EEPROM_SIZE 1

#include <WiFi.h>
#include "AsyncUDP.h"
#include <Wire.h>

//#include "Adafruit_LEDBackpack.h"
#include <EEPROM.h>

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
// Uncomment according to your hardware type
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
//#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW

// Defining size, and output pins
#define MAX_DEVICES 8
#define CS_PIN 5

MD_Parola Display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);




// network vars
bool ethConnected = false;
bool goodHeader = true;

AsyncUDP udp;


const unsigned int udpPort = 36700;
unsigned long udpLastPacketMillis = 0;
const unsigned int udpTimeout = 3000;
unsigned long udpTime = 0;      // Time remaining in seconds.
bool udpTimeNegative = false;   // If true, udpTime is time since 00:00.
int udpId = 0;                  // id of the master unit.

// LED display vars
//int ledBrightness = 15; // maxmimum brightness
//Adafruit_7segment ledDisplay = Adafruit_7segment();

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
  // 1) Confirm packet length of 3.
  //Serial.println("PACKET RECEIVED!");
  if (packet.length() == 3) { // Packet length has to be 3 bytes!

    // 2) Confirm packet header of 'IDCT:'
    //const char header[] = {'I','D','C','T',':','\0'}; //Not this protocol
    goodHeader = true;
    //Serial.print("Packet length: ");
    //Serial.println(packet.length());
    


    

    if (goodHeader == true) {
      
      //Serial.println("Packet OK! Yummy!");
  
      Serial.print("Byte 1 Minutes: ");

      byte byte1[2];
      
      byte1[0] = packet.data()[0] >> 4;  //
      byte1[1] = packet.data()[0] & 0xf;

      //newbyte = ((oldbyte << 4) & 0xf0) | ((oldbyte >> 4) & 0x0f);

      String byte1string=String(byte1[1]) + String(byte1[0]);

      char charbuf1[50];
      byte1string.toCharArray(charbuf1, 50) ;
      int minutes = atoi(charbuf1);
      Serial.println(minutes);
      
      
      Serial.print("Byte 2 Seconds: ");

      byte byte2[2];
      
      byte2[0] = packet.data()[1] >> 4;  //
      byte2[1] = packet.data()[1] & 0xf;

      String byte2string=String(byte2[1]) + String(byte2[0]);

      char charbuf2[50];
      byte2string.toCharArray(charbuf2, 50) ;
      int seconds = atoi(charbuf2);
      Serial.println(seconds);

      Serial.print("Byte 3 COLOR: ");

      String byte3string=String(packet.data()[2], HEX);
      
     char charbuf3[50];
     byte3string.toCharArray(charbuf3, 50);
     int color = atoi(charbuf3);

     Serial.println(color);
     
     Display.setTextAlignment(PA_LEFT);

     

     Display.print("EVS: " + byte1string + ":" + byte2string);


     //Add leading zeroes
      
  


      

      
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

  Display.begin();
  Display.setIntensity(0);
  Display.displayClear();


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
  /*
  //Display TESTS:
   Display.setTextAlignment(PA_LEFT);
  Display.print("ESP32");
  delay(2000);
  
  Display.setTextAlignment(PA_CENTER);
  Display.print("ESP32");
  delay(2000);

  Display.setTextAlignment(PA_RIGHT);
  Display.print("ESP32");
  delay(2000);

  Display.setTextAlignment(PA_CENTER);
  Display.setInvert(true);
  Display.print("ESP32");
  delay(2000);

  Display.setInvert(false);
  delay(2000);

  // End display tests 
  
  */
  
  
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
    //ledDisplay.writeDigitRaw(0, 0);
    //ledDisplay.writeDigitRaw(1, 0);
    //ledDisplay.writeDigitRaw(3, 0);
    //ledDisplay.writeDigitRaw(4, 0);
    //ledDisplay.writeDigitRaw(initCounter, 127);
    //ledDisplay.writeDisplay();
    initCounter += 1;
    if (initCounter == 2) {
      initCounter++;  // this jumps over the colon separating minutes and seconds
    }
  }
}

void showNoEth() {
  // Draw 'conn' to indicate we have a connection problem.
  //ledDisplay.writeDigitRaw(0, 88);
  //ledDisplay.writeDigitRaw(1, 92);
  //ledDisplay.writeDigitRaw(3, 84);
  //ledDisplay.writeDigitRaw(4, 84);
  //ledDisplay.drawColon(false);
  //ledDisplay.writeDisplay();
}

void showNoRx() {
  // Draw '--:--' to indicate we are in a ready state, but no packets with that id are being detected.
  //ledDisplay.writeDigitRaw(0, 64);
  //ledDisplay.writeDigitRaw(1, 64);
  //ledDisplay.writeDigitRaw(3, 64);
  //ledDisplay.writeDigitRaw(4, 64);
  //ledDisplay.drawColon(true);
  //ledDisplay.writeDisplay();
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
  //ledDisplay.print(displayValue, DEC);
  //ledDisplay.drawColon(true);
  // Add leading zeroes at positions 1 & 3 '-0:0-' if required.


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

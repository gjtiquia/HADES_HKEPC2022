#include <Arduino.h>
#include <string.h>

#define BLYNK_TEMPLATE_ID           "TMPLOZYkdSFA"
#define BLYNK_DEVICE_NAME           "HADES_ESP32"
#define BLYNK_FIRMWARE_VERSION        "0.1.0"
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

// must be after #define
#include "BlynkEdgent.h"

#define APP_DEBUG

#define V_SPRAYTIME V1
#define V_ONLINE V2
#define V_IR_SENSOR V3
#define V_SPRAYING V4
#define V_TEST_SPRAY V5
#define V_TERMINAL V6
#define V_TEST_CONNECTION V7

#define LED_PIN 2

WidgetTerminal terminal(V_TERMINAL);

double sprayTime;
bool connected = false;
bool online = false;

BLYNK_CONNECTED() {
  connected = true;
  digitalWrite(LED_PIN, HIGH);

  terminal.clear();
  terminal.println("Device Connected");
  terminal.flush();

  Blynk.virtualWrite(V_ONLINE, 0);

  Blynk.syncAll();
}

BLYNK_WRITE(V_SPRAYTIME) {
  sprayTime = param.asDouble();
  Serial.print((String) "Spray Time: " + sprayTime + "s\n");
  terminal.println((String) "Device Updated Spray Time: " + sprayTime + "s");
  terminal.flush();
}

BLYNK_WRITE(V_TEST_CONNECTION) {
  int button = param.asInt();

  Serial.print((String) "Test Button Pressed: " + button + "\n");
  terminal.println((String) "Device Received Test Button Press: " + button);
  terminal.flush();

  // if (button == 0) {
  //   digitalWrite(LED_PIN, LOW);
  // }
  // else if (button == 1) {
  //   digitalWrite(LED_PIN, HIGH);
  // }
}

BLYNK_WRITE(V_ONLINE) {
  int state = param.asInt();

  Serial.print((String) "Online State: " + state + "\n");
  terminal.println((String) "Device Received Online State: " + state);
  terminal.flush();

  if (state == 0) {
    online = false;
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", false);
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", false);
  }
  else if (state == 1) {
    online = true;
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", true);
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", true);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  
  BlynkEdgent.begin();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (connected && !Blynk.connected()){
    connected = false;
    digitalWrite(LED_PIN, LOW); 
  }

  BlynkEdgent.run();
}

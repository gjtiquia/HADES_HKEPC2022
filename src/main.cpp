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

WidgetTerminal terminal(V6);

double sprayTime;

BLYNK_CONNECTED() {
    terminal.println("----------------");
    terminal.println("Device Connected");
    terminal.flush();
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

  if (button == 0) {
    digitalWrite(LED_PIN, LOW);
  }
  else if (button == 1) {
    digitalWrite(LED_PIN, HIGH);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  
  BlynkEdgent.begin();

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  BlynkEdgent.run();
}

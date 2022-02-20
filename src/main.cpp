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

#define V_CONNECTED V0
#define V_SPRAYTIME V1
#define V_ONLINE V2
#define V_IR_SENSOR V3
#define V_SPRAYING V4
#define V_TEST_SPRAY

double sprayTime;

BLYNK_CONNECTED() {
    Blynk.virtualWrite(V_CONNECTED, 1);
    Blynk.syncAll();
}

BLYNK_WRITE(V_SPRAYTIME) {
  sprayTime = param.asDouble();
  Serial.print((String) "Spray Time: " + sprayTime + "s\n");
}

BLYNK_WRITE(V_CONNECTED) {
  int connected = param.asInt();
  Serial.print((String) "Connected: " + connected);
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  BlynkEdgent.begin();
}

void loop() {
  BlynkEdgent.run();
}

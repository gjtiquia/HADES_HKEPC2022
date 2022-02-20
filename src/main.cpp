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

double sprayTime;

BLYNK_CONNECTED() {
    Blynk.syncAll();
}

BLYNK_WRITE(V1) {
  sprayTime = param.asDouble();
  Serial.print((String) "Spray Time: " + sprayTime + "\n");
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

#include <Arduino.h>
#include <SimpleTimer.h>

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
#define IR_SENSOR_PIN 0 // 13
#define MOTOR_PIN 14

SimpleTimer timer;

WidgetTerminal terminal(V_TERMINAL);

double sprayTime;
bool connected = false;
bool online = false;
bool spraying = false;

void stop_spray() {
  spraying = false;
  digitalWrite(MOTOR_PIN, LOW);
  Blynk.virtualWrite(V_SPRAYING, 0);
  Blynk.virtualWrite(V_TEST_SPRAY, 0);

  Blynk.setProperty(V_ONLINE, "isDisabled", false);

  if (!online) {
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", false);
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", false);
  }
}

void spray() {
  spraying = true;
  digitalWrite(MOTOR_PIN, HIGH);
  Blynk.virtualWrite(V_SPRAYING, 1);
  Blynk.setProperty(V_ONLINE, "isDisabled", true);

  // Stop spraying after sprayTime
  timer.setTimeout(sprayTime * 1000, stop_spray);
}

BLYNK_CONNECTED() {
  connected = true;
  digitalWrite(LED_PIN, HIGH);

  terminal.clear();
  terminal.println("Device Connected");
  terminal.flush();

  // Initialize UI
  Blynk.virtualWrite(V_ONLINE, 0);
  Blynk.virtualWrite(V_IR_SENSOR, 0);
  Blynk.virtualWrite(V_SPRAYING, 0);
  Blynk.virtualWrite(V_TEST_SPRAY, 0);
  Blynk.setProperty(V_TEST_SPRAY, "isDisabled", false);
  Blynk.setProperty(V_SPRAYTIME, "isDisabled", false);
  Blynk.setProperty(V_ONLINE, "isDisabled", false);

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

BLYNK_WRITE(V_TEST_SPRAY) {
  int state = param.asInt();

  Serial.print((String) "Test Spray State: " + state + "\n");
  terminal.println((String) "Device Received Test Spray State: " + state);
  terminal.flush();

  if (state == 1) {
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", true);
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", true);
    spray();
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  
  BlynkEdgent.begin();

  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
}

void loop() {
  BlynkEdgent.run();
  timer.run();

  // Connection LED indicator
  if (connected && !Blynk.connected()){
    connected = false;
    digitalWrite(LED_PIN, LOW); 
  }

  // IR Sensor Indicator
  if (digitalRead(IR_SENSOR_PIN) == LOW) {
    Blynk.virtualWrite(V_IR_SENSOR, 1);
  }
  else if (digitalRead(IR_SENSOR_PIN) == HIGH) {
    Blynk.virtualWrite(V_IR_SENSOR, 0);
  }

  // Online Spraying
  if (online && !spraying && digitalRead(IR_SENSOR_PIN) == LOW) {
    spray();
  }
}

#include <Arduino.h>
#include <SimpleTimer.h>
#include <ESP32Servo.h>

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
#define V_SUPPLY V8

// #define BOARD BUTTON PIN in Settings.h for reboot to connect to new network
#define LED_PIN 2
#define IR_SENSOR_PIN 13
#define IR_POWER_PIN 14
#define MOTOR_PIN 26
#define WATER_POWER_PIN 33
#define WATER_SENSOR_PIN 35

#define WATER_THRESHOLD 2
#define WATER_NOTIFY_LIMIT_TIME 1800000 // in ms = 30min

SimpleTimer timer;
WidgetTerminal terminal(V_TERMINAL);
Servo myservo;

double sprayTime;
bool connected = false;
bool online = false;
bool spraying = false;
bool notified = false;
bool hasSupply = false;
int rest_pos = 160;
int spray_pos = 20;

// TO DO
// IR sensor to push mode given sensor is a toggle
// replace spray time with no. of sprays to set by user
// water sensor check every day / few hours to prevent corrosion
// calibrate water sensor (when empty)

void debug(String message) {
  Serial.print((String) message + "\n");
  terminal.println((String) message);
  terminal.flush();
}

void rest_servo() {
  myservo.detach();
  debug("Rested");
}

void stop_spray() {
  spraying = false;
  // digitalWrite(MOTOR_PIN, LOW);
  myservo.write(rest_pos);
  timer.setTimeout(sprayTime * 1000, rest_servo);

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
  // digitalWrite(MOTOR_PIN, HIGH);
  myservo.attach(MOTOR_PIN, 500, 2400); // using default min/max of 1000us and 2000us
  myservo.write(spray_pos);
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
  Blynk.virtualWrite(V_SUPPLY, 0);
  Blynk.setProperty(V_TEST_SPRAY, "isDisabled", false);
  Blynk.setProperty(V_SPRAYTIME, "isDisabled", false);
  Blynk.setProperty(V_ONLINE, "isDisabled", false);

  Blynk.syncAll();
}

BLYNK_WRITE(V_SPRAYTIME) {
  sprayTime = param.asDouble();
  debug((String) "Spray Time: " + sprayTime + "s");
}

BLYNK_WRITE(V_TEST_CONNECTION) {
  int button = param.asInt();

  debug((String) "Test Button Pressed: " + button);
}

BLYNK_WRITE(V_ONLINE) {
  int state = param.asInt();

  debug((String) "Online State: " + state);

  if (state == 0) {
    online = false;
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", false);
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", false);

    notified = false;
  }
  else if (state == 1) {
    online = true;
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", true);
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", true);
  }
}

BLYNK_WRITE(V_TEST_SPRAY) {
  int state = param.asInt();

  debug((String) "Test Spray State: " + state);

  if (state == 1) {
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", true);
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", true);
    spray();
  }
}

void offIR() {
  Blynk.virtualWrite(V_IR_SENSOR, 0);
}

void IRAM_ATTR detectIR() {
  Blynk.virtualWrite(V_IR_SENSOR, 1);
  if (online && !spraying && hasSupply) spray();

  // set off after 200ms
  timer.setTimeout(200, offIR);
}

void reset_notify() {
  notified = false;
}

void measureWater() {
  int water = analogRead(WATER_SENSOR_PIN);
  digitalWrite(WATER_POWER_PIN, LOW);

  // water = WATER_THRESHOLD + 2;

  if (water < WATER_THRESHOLD) {
    if (hasSupply) {
      Blynk.virtualWrite(V_SUPPLY, 0);
      hasSupply = false;
    }

    if (online && !notified) {
      // push
      debug("Pushed Notification");
      Blynk.logEvent("refill_disinfectant");

      // start timer for next
      notified = true;
      timer.setTimeout(WATER_NOTIFY_LIMIT_TIME, reset_notify);
    }
  }
  else {
    if (!hasSupply) {
      Blynk.virtualWrite(V_SUPPLY, 1);
      hasSupply = true;
    }
    
    if (notified = true) {
      notified = false;
    }
  }
}

void detectWater() {
  // measure water resistance
  digitalWrite(WATER_POWER_PIN, HIGH);
  timer.setTimeout(10, measureWater);
}

void IRAM_ATTR buttonPressed() {
  debug("Boot pressed");
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  
  BlynkEdgent.begin();

  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(MOTOR_PIN, 500, 2400); // using default min/max of 1000us and 2000us
  myservo.write(rest_pos);
  timer.setTimeout(1000, rest_servo);

  pinMode(IR_SENSOR_PIN, INPUT_PULLUP); // because the IR Sensor gives either 0V or ??V
  pinMode(IR_POWER_PIN, OUTPUT);
  pinMode(WATER_SENSOR_PIN, INPUT);
  pinMode(WATER_POWER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  // test boot button
  // pinMode(0, INPUT);
  // attachInterrupt(digitalPinToInterrupt(0), buttonPressed, CHANGE);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(WATER_POWER_PIN, LOW);
  digitalWrite(IR_POWER_PIN, HIGH);

  // attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), detectIR, CHANGE);
}

void loop() {
  BlynkEdgent.run();
  timer.run();

  // Connection LED indicator
  if (connected && !Blynk.connected()){
    connected = false;
    digitalWrite(LED_PIN, LOW); 
  }

  // Detect Water
  detectWater();
}

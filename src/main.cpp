#include <Arduino.h>
#include <SimpleTimer.h>
#include <ESP32Servo.h>

#define BLYNK_TEMPLATE_ID           "TMPLOZYkdSFA"
#define BLYNK_DEVICE_NAME           "HADES_ESP32"
#define BLYNK_FIRMWARE_VERSION        "0.1.0"
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG
#define APP_DEBUG

// must be after #define
#include "BlynkEdgent.h"

#define V_SPRAYTIME V1
#define V_ONLINE V2
#define V_IR_SENSOR V3
#define V_SPRAYING V4
#define V_TEST_SPRAY V5
#define V_TERMINAL V6
#define V_TEST_CONNECTION V7
#define V_SUPPLY V8
#define V_NOTIFICATION_TOGGLE V9
#define V_MEASURE_WATER V10
#define V_SET_WATER_THRESHOLD V11
#define V_WATER_THRESHOLD V12
#define V_NUMBER_OF_SPRAY V13
#define V_SPRAY_AMOUNT V14

// #define BOARD BUTTON PIN in Settings.h for reboot to connect to new network
#define IR_SENSOR_PIN 13
#define IR_POWER_PIN 14
#define MOTOR_PIN 26
#define WATER_POWER_PIN 33
#define WATER_SENSOR_PIN 35

#define WATER_NOTIFY_LIMIT_TIME 1800000 // in ms = 30min
#define WATER_DETECT_FREQUENCY 1800000 // in ms = 30min

#define IR_TIMEOUT 200

SimpleTimer timer;
WidgetTerminal terminal(V_TERMINAL);
Servo myservo;

int sprayNum;
int sprayCount = 0;
double sprayTime;
double sprayRatio;
bool connected = false;
bool online = false;
bool spraying = false;
bool notified = false;
bool hasSupply = false;
bool currentIR = false;
bool isIRActivated = false;
bool notificationToggle = true;
int rest_pos = 160;
int spray_pos = 20;
int water_threshold;

// Function for displaying strings in the console and in the terminal widget.
void debug(String message) {
  Serial.print((String) message + "\n");
  terminal.println((String) message);
  terminal.flush();
}

// Function to rest and detach the servo motor.
// Note: Needs to be above stop_spray() as it is referenced there.
void rest_servo() {
  myservo.detach();
  debug("Rested");
}

// Function to stop spraying by resetting the servo arm position.
// Will reset sprayCount and app widgets when reached the target number of sprays.
// Note: Needs to be above sprayOnce() as it is referenced there.
void stop_spray() {
  spraying = false;
  myservo.write(rest_pos);

  if (sprayCount == sprayNum) {
    sprayCount = 0;

    timer.setTimeout(sprayTime * 1000, rest_servo);

    Blynk.virtualWrite(V_SPRAYING, 0);
    Blynk.virtualWrite(V_TEST_SPRAY, 0);
    Blynk.setProperty(V_ONLINE, "isDisabled", false);

    if (!online) {
      Blynk.setProperty(V_TEST_SPRAY, "isDisabled", false);
      Blynk.setProperty(V_SPRAYTIME, "isDisabled", false);
      Blynk.setProperty(V_NUMBER_OF_SPRAY, "isDisabled", false);
      Blynk.setProperty(V_SPRAY_AMOUNT, "isDisabled", false);
    }
  }
}

// Function to spray once by setting the servo arm to the desired position.
// Note: Needs to be above spray() as it is referenced there.
void sprayOnce() {
  spraying = true;
  sprayCount++;
  debug((String) "Spray Count: " + sprayCount);
  myservo.attach(MOTOR_PIN, 500, 2400); // using default min/max of 1000us and 2000us
  myservo.write(spray_pos + (rest_pos - spray_pos) * (1 - sprayRatio));
  Blynk.virtualWrite(V_SPRAYING, 1);
  Blynk.setProperty(V_ONLINE, "isDisabled", true);

  // Stop spraying after sprayTime
  timer.setTimeout(sprayTime * 1000, stop_spray);
}

// Function to instruct the servo to spray for a certain number of times
// Note: This funciton is referenced in multiple areas of the code so is placed high above.
void spray() {
  debug((String) "Will spray " + sprayNum + " time(s)");

  sprayOnce();
  if (sprayNum > 1) {
    timer.setTimer(sprayTime * 1000 * 2.2, sprayOnce, sprayNum - 1);
  }
}

// Called when ESP32 is connected to the Blynk.Cloud
// Initializes the Blynk.App widget UI and also gets the parameters for the app to work.
BLYNK_CONNECTED() {
  connected = true;

  terminal.clear();
  terminal.println("Device Connected");
  terminal.flush();

  // Initialize UI
  Blynk.virtualWrite(V_ONLINE, 0);
  Blynk.virtualWrite(V_IR_SENSOR, 0);
  Blynk.virtualWrite(V_SPRAYING, 0);
  Blynk.virtualWrite(V_TEST_SPRAY, 0);
  Blynk.virtualWrite(V_SUPPLY, 0);
  Blynk.virtualWrite(V_NOTIFICATION_TOGGLE, 0);
  Blynk.setProperty(V_TEST_SPRAY, "isDisabled", false);
  Blynk.setProperty(V_SPRAYTIME, "isDisabled", false);
  Blynk.setProperty(V_NUMBER_OF_SPRAY, "isDisabled", false);
  Blynk.setProperty(V_SPRAY_AMOUNT, "isDisabled", false);
  Blynk.setProperty(V_ONLINE, "isDisabled", false);

  Blynk.syncAll();

  // Redundency
  Blynk.syncVirtual(V_SPRAY_AMOUNT, V_NUMBER_OF_SPRAY, V_WATER_THRESHOLD);
}

// Calls whenever the spray time slider changes value.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
BLYNK_WRITE(V_SPRAYTIME) {
  sprayTime = param.asDouble();
  debug((String) "Spray Time: " + sprayTime + "s");
}

// Calls whenever the spray amount slider changes value.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
BLYNK_WRITE(V_SPRAY_AMOUNT) {
  sprayRatio = param.asDouble();

  int angle = spray_pos + (rest_pos - spray_pos) * (1 - sprayRatio);

  debug(
    (String) "Spray Amount: " + sprayRatio * 100 + "% [" 
    + rest_pos + " -> " + angle + " -> " + spray_pos + "]"
  );
}

// Calls whenever the number of spray slider changes value.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
BLYNK_WRITE(V_NUMBER_OF_SPRAY) {
  sprayNum = param.asDouble();
  debug((String) "Number of Spray: " + sprayNum);
}

// Calls whenever the test connection button is pressed.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
BLYNK_WRITE(V_TEST_CONNECTION) {
  int button = param.asInt();

  debug((String) "Test Button Pressed: " + button);
}

// Calls whenever the online toggle button is pressed.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
// Enables and disables certain widgets when online or offine.
BLYNK_WRITE(V_ONLINE) {
  int state = param.asInt();

  debug((String) "Online State: " + state);

  if (state == 0) {
    online = false;
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", false);
    Blynk.setProperty(V_NUMBER_OF_SPRAY, "isDisabled", false);
    Blynk.setProperty(V_SPRAY_AMOUNT, "isDisabled", false);
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", false);

    notified = false;
  }
  else if (state == 1) {
    online = true;
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", true);
    Blynk.setProperty(V_NUMBER_OF_SPRAY, "isDisabled", true);
    Blynk.setProperty(V_SPRAY_AMOUNT, "isDisabled", true);
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", true);
  }
}

// Calls whenever the notification toggle button is pressed.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
BLYNK_WRITE(V_NOTIFICATION_TOGGLE) {
  int state = param.asInt();

  debug((String) "Notification Toggled: " + state);

  if (state == 0) {
    notificationToggle = false;
  }
  else if (state == 1) {
    notificationToggle = true;
  }
}

// Calls whenever the spray now or test spray button is pressed.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
BLYNK_WRITE(V_TEST_SPRAY) {
  int state = param.asInt();

  debug((String) "Test Spray State: " + state);

  if (state == 1) {
    Blynk.setProperty(V_TEST_SPRAY, "isDisabled", true);
    Blynk.setProperty(V_SPRAYTIME, "isDisabled", true);
    Blynk.setProperty(V_NUMBER_OF_SPRAY, "isDisabled", true);
    Blynk.setProperty(V_SPRAY_AMOUNT, "isDisabled", true);
    spray();
  }
}

// Calls whenever the value of the measured water threshold changes.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
BLYNK_WRITE(V_WATER_THRESHOLD) {
  water_threshold = param.asInt();
  debug((String) "Water Threshold: " + water_threshold);
}

// Function to measure the water level with the water sensor and set the water threshold.
// Note: Needs to be above BLYNK_WRITE(V_SET_WATER_THRESHOLD) as it is referenced there.
void setWaterThreshold() {
  water_threshold = analogRead(WATER_SENSOR_PIN);
  Blynk.virtualWrite(V_WATER_THRESHOLD, water_threshold);
  digitalWrite(WATER_POWER_PIN, LOW);
  debug((String) "Water Threshold Set as: " + water_threshold);
}

// Calls whenever the set water threshold button is pressed.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
BLYNK_WRITE(V_SET_WATER_THRESHOLD) {
  int state = param.asInt();
  if (state == 1) {
    digitalWrite(WATER_POWER_PIN, HIGH);
    timer.setTimeout(10, setWaterThreshold);
  }
}

// Function to measure the water level with the water sensor.
// Note: Needs to be above BLYNK_WRITE(V_MEASURE_WATER) as it is referenced there.
void measureWaterThreshold() {
  int measuredWater = analogRead(WATER_SENSOR_PIN);
  Blynk.virtualWrite(V_WATER_THRESHOLD, measuredWater);
  digitalWrite(WATER_POWER_PIN, LOW);
  debug((String) "Water Measured as: " + measuredWater);
}

// Calls whenever the measure water button is pressed.
// Also calls with Blynk.syncAll() and Blynk.syncVirtual().
BLYNK_WRITE(V_MEASURE_WATER) {
  int state = param.asInt();
  if (state == 1) {
    digitalWrite(WATER_POWER_PIN, HIGH);
    timer.setTimeout(10, measureWaterThreshold);
  }
}

// Function to check the IR sensor input and spray accordingly
void detectIR() {
  // check if IR changed
  if (currentIR != digitalRead(IR_SENSOR_PIN)) {
    currentIR = digitalRead(IR_SENSOR_PIN);
    // if (online && !spraying && hasSupply) spray();
    if (online && !spraying && sprayCount == 0) spray();
  }
}

// Function to reset the notified flag after a specified amount of time
// Note: Needs to be above measureWater() as it is referenced there.
void reset_notify() {
  notified = false;
}

// Function to measure the water, display the supply status 
// and push notifications based on the water threshold.
// Measures every specified time period.
// Note:  Needs to be above detectWater() as it is referenced there.
void measureWater() {
  int water = analogRead(WATER_SENSOR_PIN);
  debug((String) "Water Measured as: " + water);
  digitalWrite(WATER_POWER_PIN, LOW);

  if (water < water_threshold) {
    if (hasSupply) {
      Blynk.virtualWrite(V_SUPPLY, 0);
      hasSupply = false;
    }

    if (online && !notified && notificationToggle) {
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
  }
}

// Function to first turn on the water sensor power pin before measuring water.
void detectWater() {
  // measure water resistance
  digitalWrite(WATER_POWER_PIN, HIGH);
  timer.setTimeout(10, measureWater);
}

// Begin serial communication and Blynk WiFi connection
// Setup servos and GPIO pins
// Begins the water measuring interval
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

  pinMode(IR_SENSOR_PIN, INPUT_PULLUP); // The IR Sensor gives either 0V or ??V
  pinMode(IR_POWER_PIN, OUTPUT);
  pinMode(WATER_SENSOR_PIN, INPUT);
  pinMode(WATER_POWER_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(WATER_POWER_PIN, LOW);
  digitalWrite(IR_POWER_PIN, HIGH);

  currentIR = digitalRead(IR_SENSOR_PIN);

  // Start to Detect Water every 30min
  detectWater();
  timer.setInterval(WATER_DETECT_FREQUENCY, detectWater);
}

// Runs the WiFi connectino
// Runs the timer
// Checks if connected
// Checks for IR sensor input
void loop() {
  BlynkEdgent.run();
  timer.run();

  // Connected flag
  if (connected && !Blynk.connected()) {
    connected = false;
  }

  // Detect IR
  detectIR();
}

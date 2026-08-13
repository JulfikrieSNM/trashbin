#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// WiFi Credentials
// =====================================================

const char* ssid = "Your_Wifi_Name";
const char* password = "Your_Password";

// =====================================================
// Google Form URL
// =====================================================
// In the link change the viewForm into formResponse

const String form_url =
"your_spreadsheet_Link";

// =====================================================
// ULTRASONIC SENSOR PINS
// =====================================================

#define TRIG_PIN 5
#define ECHO_PIN 18

// =====================================================
// FULL BIN THRESHOLD
// =====================================================

#define FULL_THRESHOLD_CM 5.0

// =====================================================
// LCD
// =====================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================================================
// SECURE WIFI CLIENT
// =====================================================

WiFiClientSecure secureClient;

// =====================================================
// HW-870 LINE SENSOR PINS
// =====================================================

#define LEFT_SENSOR_PIN 34
#define RIGHT_SENSOR_PIN 35

// =====================================================
// MOTOR DRIVER PINS
// L298N
// =====================================================

// LEFT MOTOR
#define IN1 25
#define IN2 26

// RIGHT MOTOR
#define IN3 27
#define IN4 14

// =====================================================
// HW-870 SENSOR LOGIC
// =====================================================
// Assuming:
// LOW  = BLACK
// HIGH = WHITE
//
// If your HW-870 gives HIGH when detecting black,
// change to:
//
// #define BLACK HIGH
// #define WHITE LOW
// =====================================================

#define BLACK LOW
#define WHITE HIGH

// =====================================================
// TIMING
// =====================================================

unsigned long previousSendTime = 0;

const unsigned long sendInterval = 5000;

// =====================================================
// ROBOT STATE
// =====================================================

bool robotActive = false;

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // ===================================================
  // ULTRASONIC SENSOR
  // ===================================================

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // ===================================================
  // HW-870 LINE SENSORS
  // ===================================================

  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);

  // ===================================================
  // MOTOR DRIVER
  // ===================================================

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Make sure robot is stopped
  stopMotors();

  // ===================================================
  // LCD
  // ===================================================

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Smart Bin");

  lcd.setCursor(0, 1);
  lcd.print("Connecting...");

  // ===================================================
  // CONNECT TO WIFI
  // ===================================================

  WiFi.begin(ssid, password);

  Serial.println();
  Serial.println("================================");
  Serial.println("       SMART BIN SYSTEM");
  Serial.println("================================");

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Skip SSL certificate verification
  secureClient.setInsecure();

  // ===================================================
  // LCD
  // ===================================================

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");

  delay(1500);

  lcd.clear();

  // ===================================================
  // SYSTEM READY
  // ===================================================

  Serial.println();
  Serial.println("System Ready!");
  Serial.println("Robot: DEACTIVATED");
  Serial.println("================================");
}

// =====================================================
// READ DISTANCE
// =====================================================

float readDistance() {

  // Clear trigger
  digitalWrite(TRIG_PIN, LOW);

  delayMicroseconds(2);

  // Send ultrasonic pulse
  digitalWrite(TRIG_PIN, HIGH);

  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  // Read echo
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  // No echo received
  if (duration == 0) {

    return -1;
  }

  // Calculate distance
  float distanceCm = duration * 0.0343 / 2;

  return distanceCm;
}

// =====================================================
// SEND STATUS TO GOOGLE SHEETS
// =====================================================

void sendStatusToGoogleSheets(String status) {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi not connected!");

    return;
  }

  HTTPClient http;

  // Build Google Form URL
  String finalURL = form_url + status + "&submit=Submit";

  Serial.println();
  Serial.println("Sending to Google Sheets...");
  Serial.println("Status: " + status);

  // Start HTTPS connection
  http.begin(secureClient, finalURL);

  // Send GET request
  int httpCode = http.GET();

  // Check result
  if (httpCode > 0) {

    Serial.print("Google Form HTTP Code: ");
    Serial.println(httpCode);

    if (httpCode == 200) {

      Serial.println("Data sent successfully!");
    }

  } else {

    Serial.print("Failed to send data. Error: ");
    Serial.println(http.errorToString(httpCode));
  }

  // Close connection
  http.end();
}

// =====================================================
// STOP MOTORS
// =====================================================

void stopMotors() {

  // Left motor
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  // Right motor
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// =====================================================
// MOVE FORWARD
// =====================================================

void moveForward() {

  // Left motor forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Right motor forward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// =====================================================
// TURN LEFT
// =====================================================

void turnLeft() {

  // Left motor stop
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  // Right motor forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// =====================================================
// TURN RIGHT
// =====================================================

void turnRight() {

  // Left motor forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Right motor stop
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// =====================================================
// LINE FOLLOWING
// =====================================================

void lineFollow() {

  // Read HW-870 sensors
  int leftSensor = digitalRead(LEFT_SENSOR_PIN);

  int rightSensor = digitalRead(RIGHT_SENSOR_PIN);

  // ===================================================
  // PRINT SENSOR VALUES
  // ===================================================

  Serial.print("Left Sensor: ");
  Serial.print(leftSensor);

  Serial.print(" | Right Sensor: ");
  Serial.println(rightSensor);

  // ===================================================
  // BOTH BLACK
  // MOVE FORWARD
  // ===================================================

  if (leftSensor == BLACK && rightSensor == BLACK) {

    moveForward();

    Serial.println("Robot Movement: FORWARD");
  }

  // ===================================================
  // LEFT BLACK / RIGHT WHITE
  // TURN LEFT
  // ===================================================

  else if (leftSensor == BLACK && rightSensor == WHITE) {

    turnLeft();

    Serial.println("Robot Movement: TURN LEFT");
  }

  // ===================================================
  // LEFT WHITE / RIGHT BLACK
  // TURN RIGHT
  // ===================================================

  else if (leftSensor == WHITE && rightSensor == BLACK) {

    turnRight();

    Serial.println("Robot Movement: TURN RIGHT");
  }

  // ===================================================
  // BOTH WHITE
  // LINE LOST
  // ===================================================

  else if (leftSensor == WHITE && rightSensor == WHITE) {

    stopMotors();

    Serial.println("Robot Movement: STOP - LINE LOST");
  }
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // READ ULTRASONIC DISTANCE
  // ===================================================

  float distanceCm = readDistance();

  // ===================================================
  // DETERMINE BIN STATUS
  // ===================================================

  String binStatus;

  if (distanceCm > 0 && distanceCm <= FULL_THRESHOLD_CM) {

    binStatus = "FULL";

  } else {

    binStatus = "OK";
  }

  // ===================================================
  // ROBOT ACTIVATION
  // ===================================================

  if (binStatus == "FULL") {

    robotActive = true;

  } else {

    robotActive = false;

    // Stop robot if bin is OK
    stopMotors();
  }

  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.println();
  Serial.println("================================");
  Serial.println("        SMART BIN STATUS");
  Serial.println("================================");

  // Distance
  Serial.print("Distance: ");

  if (distanceCm < 0) {

    Serial.println("SENSOR ERROR");

  } else {

    Serial.print(distanceCm, 1);
    Serial.println(" cm");
  }

  // Bin status
  Serial.print("Bin Status: ");
  Serial.println(binStatus);

  // Robot activation
  Serial.print("Robot: ");

  if (robotActive) {

    Serial.println("ACTIVATED");

  } else {

    Serial.println("DEACTIVATED");
  }

  Serial.println("================================");

  // ===================================================
  // LCD DISPLAY
  // ===================================================

  lcd.setCursor(0, 0);

  lcd.print("Dist: ");

  if (distanceCm < 0) {

    lcd.print("ERROR       ");

  } else {

    lcd.print(distanceCm, 1);
    lcd.print(" cm     ");
  }

  lcd.setCursor(0, 1);

  if (binStatus == "FULL") {

    lcd.print("Status: FULL   ");

  } else {

    lcd.print("Status: OK     ");
  }

  // ===================================================
  // ROBOT LINE FOLLOWING
  // ===================================================

  if (robotActive) {

    Serial.println("Line Following: ACTIVE");

    lineFollow();

  } else {

    stopMotors();
  }

  // ===================================================
  // SEND TO GOOGLE SHEETS
  // EVERY 5 SECONDS
  // ===================================================

  if (millis() - previousSendTime >= sendInterval) {

    previousSendTime = millis();

    sendStatusToGoogleSheets(binStatus);
  }

  // Small delay
  delay(100);
}

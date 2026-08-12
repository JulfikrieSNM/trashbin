
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===============================
// WiFi Credentials
// ===============================
const char* ssid = "joule";
const char* password = "Jul12345";

// ===============================
// Google Form URL
// ===============================
// Example:
// https://docs.google.com/forms/d/e/XXXXXXXX/formResponse?entry.123456789=
//
// Put your Google Form "formResponse" URL here.
// Make sure the entry ID is for the STATUS question.
const String form_url =
  "https://docs.google.com/forms/d/e/1FAIpQLSeJImxu4moiK-RXo0cNG7le8UdYwZIxzWATTcRJ5acl-hZ51A/formResponse?usp=pp_url&entry.957340545=";

// ===============================
// Ultrasonic Sensor Pins
// ===============================
#define TRIG_PIN 5
#define ECHO_PIN 18

// ===============================
// Full Bin Threshold
// ===============================
#define FULL_THRESHOLD_CM 5.0

// ===============================
// LCD
// ===============================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===============================
// Secure WiFi Client
// ===============================
WiFiClientSecure secureClient;

// ===============================
// Timing
// ===============================
unsigned long previousSendTime = 0;
const unsigned long sendInterval = 5000; // 5 seconds


// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  // Ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Smart Bin");
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");

  // ===============================
  // Connect WiFi
  // ===============================
  WiFi.begin(ssid, password);

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

  // LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");

  delay(1500);

  lcd.clear();
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

  // If no echo
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
  Serial.println("URL: " + finalURL);

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
// LOOP
// =====================================================
void loop() {

  // ===============================
  // Read ultrasonic distance
  // ===============================
  float distanceCm = readDistance();

  // ===============================
  // Serial Monitor
  // ===============================
  Serial.print("Distance: ");

  if (distanceCm < 0) {

    Serial.println("Sensor Error");

  } else {

    Serial.print(distanceCm, 1);
    Serial.println(" cm");
  }


  // ===============================
  // Determine bin status
  // ===============================
  String binStatus;

  if (distanceCm > 0 && distanceCm <= FULL_THRESHOLD_CM) {

    binStatus = "FULL";

  } else {

    binStatus = "OK";
  }


  // ===============================
  // LCD Display
  // ===============================

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


  // ===============================
  // Send to Google Sheets
  // Every 5 seconds
  // ===============================
  if (millis() - previousSendTime >= sendInterval) {

    previousSendTime = millis();

    sendStatusToGoogleSheets(binStatus);
  }


  // Small delay
  delay(500);
}

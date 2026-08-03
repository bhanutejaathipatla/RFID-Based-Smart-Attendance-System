#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

#define RST_PIN  D3
#define SS_PIN   D4
#define BUZZER   D8

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x3F, 16, 2);

const unsigned long WIFI_TIMEOUT_MS   = 15000;
const unsigned long DEBOUNCE_MS       = 8000;
const uint8_t       HTTP_MAX_RETRIES  = 3;

String   lastUID = "";
unsigned long lastScanTime = 0;

void setup() {
  Serial.begin(9600);

  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Initializing  ");

  pinMode(BUZZER, OUTPUT);

  SPI.begin();
  mfrc522.PCD_Init();

  connectWiFi();
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    Serial.print(".");
    delay(300);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  WiFi Connected");
    delay(1000);
  } else {
    Serial.println("\nWiFi connect failed — continuing offline, will retry on scan.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" No WiFi (offline)");
    delay(1000);
  }
}

String getUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
    delay(150);
  }
}

String sendAttendance(String uid) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) return "";
  }

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  String url = String(SHEET_URL) + "?uid=" + uid + "&key=" + API_KEY;

  for (uint8_t attempt = 1; attempt <= HTTP_MAX_RETRIES; attempt++) {
    HTTPClient https;
    if (https.begin(*client, url)) {
      int httpCode = https.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        https.end();
        return payload;
      }
      Serial.printf("Attempt %d failed, HTTP code: %d\n", attempt, httpCode);
      https.end();
    }
    delay(500);
  }
  return "";
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Scan your Card ");

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial())   return;

  String uid = getUID();
  Serial.println("Card UID: " + uid);

  if (uid == lastUID && millis() - lastScanTime < DEBOUNCE_MS) {
    mfrc522.PICC_HaltA();
    return;
  }
  lastUID = uid;
  lastScanTime = millis();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Checking...  ");

  String response = sendAttendance(uid);

  lcd.clear();
  lcd.setCursor(0, 0);
  if (response == "") {
    lcd.print("  Network Error ");
    beep(1);
  } else if (response.startsWith("OK:")) {
    String name = response.substring(3);
    lcd.print("Hi " + name + "!");
    lcd.setCursor(0, 1);
    lcd.print(" Marked Present ");
    beep(2);
  } else if (response == "DUPLICATE") {
    lcd.print(" Already Marked ");
    beep(1);
  } else if (response == "UNKNOWN") {
    lcd.print("  Unknown Card  ");
    beep(3);
  } else {
    lcd.print("  Server Error  ");
    beep(1);
  }

  delay(1500);
  mfrc522.PICC_HaltA();
}

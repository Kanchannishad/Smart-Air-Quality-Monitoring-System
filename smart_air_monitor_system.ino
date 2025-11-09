#include <SimpleBLE.h>

#include <Blynk.h>

// ---------------------------------------------------------------------
// SMART HOME AIR VENTILATION SYSTEM with AUTO + MANUAL + NOTIFICATIONS + STATUS
// ---------------------------------------------------------------------

#define BLYNK_TEMPLATE_ID "TMPL3UiGGQI26"
#define BLYNK_TEMPLATE_NAME "Smart home air ventilation"
#define BLYNK_AUTH_TOKEN "vI1UXaZ6gdhz2LgWBhtaXwrRXOtXC92i"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "DHT.h"

// ----- WiFi Credentials -----
char ssid[] = "Nothing Phone (3a)_6032";    
char pass[] = "kanchan123";                

// ----- DHT11 SETUP -----
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ----- MQ135 SETUP -----
const int mq135Pin = 34;
int airQualityValue = 0;

// ----- RELAY + LED -----
#define RELAY_PIN 14
#define LED_PIN 2   // Onboard LED on ESP32

// ----- Thresholds -----
#define TEMP_THRESHOLD 30.0
#define GAS_THRESHOLD 1500

// ----- Blynk Virtual Pins -----
#define VPIN_TEMP V0
#define VPIN_GAS V1
#define VPIN_HUMIDITY V2
#define VPIN_FAN_STATUS V3
#define VPIN_MANUAL_FAN V4

// ----- Variables -----
bool manualControl = false;
bool fanState = false; // Track if fan is currently ON or OFF
BlynkTimer timer;
unsigned long lastAlertTime = 0;

// ----- Function to send alerts (with spam protection) -----
void sendAlert(String msg) {
  if (millis() - lastAlertTime > 30000) {  // wait 30 sec between alerts
    Blynk.logEvent("alert", msg);
    lastAlertTime = millis();
    Serial.println("📱 Notification sent: " + msg);
  }
}

// ----- Function to update fan status everywhere -----
void updateFanStatus(bool state) {
  fanState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  digitalWrite(LED_PIN, state ? HIGH : LOW);
  Blynk.virtualWrite(VPIN_FAN_STATUS, state ? 1 : 0);
  Serial.println(state ? "🌀 Fan ON" : "💤 Fan OFF");
}

// ----- Read Sensors -----
void readSensors() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  airQualityValue = analogRead(mq135Pin);

  if (isnan(h) || isnan(t)) {
    Serial.println("❌ Failed to read from DHT11!");
    return;
  }

  Serial.println("-------------------------------");
  Serial.print("🌡 Temp: "); Serial.print(t); Serial.println(" °C");
  Serial.print("💨 Gas Value: "); Serial.println(airQualityValue);
  Serial.print("💧 Humidity: "); Serial.print(h); Serial.println(" %");

  // ----- AUTO MODE -----
  if (!manualControl) {
    if ((t > TEMP_THRESHOLD) || (airQualityValue > GAS_THRESHOLD)) {
      if (!fanState) { // only change if fan was OFF
        updateFanStatus(true);
        sendAlert("⚠️ Poor air quality or high temperature! Fan turned ON.");
      }
    } else {
      if (fanState) { // only change if fan was ON
        updateFanStatus(false);
      }
    }
  }

  // ----- Send Data to Blynk -----
  Blynk.virtualWrite(VPIN_TEMP, t);
  Blynk.virtualWrite(VPIN_GAS, airQualityValue);
  Blynk.virtualWrite(VPIN_HUMIDITY, h);
}

// ----- Manual Fan Control -----
BLYNK_WRITE(VPIN_MANUAL_FAN) {
  int value = param.asInt();
  manualControl = (value == 1);

  if (manualControl) {
    updateFanStatus(true);
    Serial.println("🟢 Manual Control: Fan ON");
  } else {
    updateFanStatus(false);
    Serial.println("🔴 Manual Control: Fan OFF");
  }
}

// ----- Setup -----
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(mq135Pin, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  updateFanStatus(false); // Fan off at start

  Serial.println("✅ Starting Smart Home Air Ventilation with Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, readSensors);  // every 2 seconds
}

// ----- Loop -----
void loop() {
  Blynk.run();
  timer.run();
}
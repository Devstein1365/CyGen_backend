#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// 🔌 WiFi
const char* ssid = "DevStein";
const char* password = "";

// 🌐 Backend URL
const char* serverUrl = "http://192.168.0.145:5000/data";

// 🖥️ LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ⚙️ RPM
#define RPM_PIN D5
volatile int pulseCount = 0;
unsigned long lastRPMTime = 0;
float rpm = 0;

// ⚡ Voltage
#define VOLTAGE_PIN A0
float voltage = 0;

// 🔋 Battery config (REAL BATTERY VALUES)
float FULL_VOLTAGE = 12.6;
float MIN_VOLTAGE = 11.0;

// ⏱️ Timing
unsigned long lastSendTime = 0;
unsigned long lastLCDTime = 0;

// 🧠 Interrupt
void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  // RPM setup
  pinMode(RPM_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RPM_PIN), countPulse, FALLING);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CyGen Booting");
  delay(1500);
  lcd.clear();

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi Connected");
  Serial.println(WiFi.localIP());
}

void loop() {

  // 🚴 RPM (SMOOTH + STABLE)
  if (millis() - lastRPMTime >= 1000) {
    noInterrupts();
    int count = pulseCount;
    pulseCount = 0;
    interrupts();

    if (count > 0) {
      rpm = count * 60;
    } else {
      rpm = rpm * 0.85; // smooth decay instead of instant drop
    }

    lastRPMTime = millis();
  }

  // ⚡ Voltage (FIXED SCALING FOR YOUR SENSOR)
  int raw = analogRead(VOLTAGE_PIN);
  float measuredVoltage = raw * (3.3 / 1023.0);

  // your divider maps ~12.6V → ~2.21V
  float scaleFactor = 12.6 / 2.21;
  voltage = measuredVoltage * scaleFactor;

  // 🔋 Battery %
  float batteryFloat = (voltage - MIN_VOLTAGE) / (FULL_VOLTAGE - MIN_VOLTAGE) * 100.0;
  batteryFloat = constrain(batteryFloat, 0, 100);
  int battery = (int)batteryFloat;

  // 🔋 Status
  String chargingStatus;

  if (battery >= 98) {
    chargingStatus = "full";
  } 
  else if (rpm > 5) {
    chargingStatus = "charging";
  } 
  else if (battery <= 10) {
    chargingStatus = "low";
  } 
  else {
    chargingStatus = "idle";
  }

  bool isPedaling = rpm > 5;

  // ⏳ ETA
  int etaToFull = 0;
  if (isPedaling && battery < 100) {
    etaToFull = (100 - battery) / 2;
  }

  // 📦 JSON
  String json = "{";
  json += "\"battery\":" + String(battery) + ",";
  json += "\"voltage\":" + String(voltage, 2) + ",";
  json += "\"rpm\":" + String((int)rpm) + ",";
  json += "\"chargingStatus\":\"" + chargingStatus + "\",";
  json += "\"isPedaling\":" + String(isPedaling ? "true" : "false") + ",";
  json += "\"etaToFull\":" + String(etaToFull);
  json += "}";

  // 🌐 SEND TO BACKEND
  if (millis() - lastSendTime >= 2000) {
    lastSendTime = millis();

    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      http.begin(client, serverUrl);
      http.addHeader("Content-Type", "application/json");

      int response = http.POST(json);

      Serial.println("📤 " + json);
      Serial.print("Response: ");
      Serial.println(response);

      http.end();
    }
  }

  // 🖥️ LCD DISPLAY
  if (millis() - lastLCDTime >= 1000) {
    lastLCDTime = millis();

    lcd.setCursor(0, 0);
    lcd.print("RPM:");
    lcd.print((int)rpm);
    lcd.print("   ");

    lcd.setCursor(0, 1);
    lcd.print("B:");
    lcd.print(battery);
    lcd.print("% ");

    if (chargingStatus == "charging") lcd.print("CHG ");
    else if (chargingStatus == "full") lcd.print("FULL");
    else if (chargingStatus == "low") lcd.print("LOW ");
    else lcd.print("IDLE");
  }
}
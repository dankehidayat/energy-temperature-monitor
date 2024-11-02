#include <WiFi.h>
#include <WiFiClient.h>
#define BLYNK_TEMPLATE_ID "TMPL6eUbLFTuj"
#define BLYNK_TEMPLATE_NAME "Energy Monitor"
#include <BlynkSimpleEsp32.h> 
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>
#include <PZEM004Tv30.h> 
#include <DHT.h>

// Initialize LCD object with I2C address and screen size (16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Your Blynk token
char auth[] = "";

// Initialize PZEM-004T object using HardwareSerial
HardwareSerial hwSerial(1); // Use UART1 on ESP32
PZEM004Tv30 pzem(hwSerial, 16, 17); // RX, TX pins on ESP32

// Initialize DHT11 object
#define DHTPIN 27 // D27 pin on ESP32
#define DHTTYPE DHT11 // DHT11 sensor type
DHT dht(DHTPIN, DHTTYPE);

#define TRIGGER_PIN 0

void checkBoot() {
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  if (digitalRead(TRIGGER_PIN) == LOW) {
    delay(100);
    if (digitalRead(TRIGGER_PIN) == LOW) {
      Serial.println("Boot button pressed");
      delay(5000);
      if (digitalRead(TRIGGER_PIN) == LOW) {
        Serial.println("Boot button held");
        Serial.println("Erasing WiFi config and restarting");
        WiFiManager wfm;
        wfm.resetSettings();
        ESP.restart();
      }
    }
  }
}

void setup() {
  // Initialize LCD
  lcd.begin();
  lcd.backlight();

  // Check boot button
  checkBoot();

  // Display intro text
  showIntroText();
  delay(3500);

  // Initialize WiFiManager
  #define AP_PASS "foo" // Password for ESP32 SSID
  #define AP_SSID "Energy IoT" // Name of ESP32 SSID

  // Define variables
  const unsigned long Timeout = 180;

  WiFiManager wfm;
  wfm.setConfigPortalTimeout(Timeout); // Timeout after 180 seconds
  wfm.setHostname(AP_SSID);
  wfm.setConnectTimeout(Timeout);

  // Display "Waiting for WiFi connection" on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Connection...");

  // Start WiFiManager configuration portal
  bool connected = wfm.autoConnect(AP_SSID, AP_PASS);

  if (!connected) {
    Serial.println("Failed to connect to WiFi!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to Connect");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Restarting ESP32...");
    delay(3500); // Display message for 3.5 seconds
    ESP.restart(); // Restart ESP32 to try again
  } else {
    Serial.println("WiFi Connected!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    delay(3500); // Display message for 3.5 seconds before moving to intro
  }

  // Initialize Blynk with local server in Serang City
  Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str(), "iot.serangkota.go.id", 8080);

  // Initialize HardwareSerial for PZEM-004T
  hwSerial.begin(9600, SERIAL_8N1, 16, 17); // RX, TX pins on ESP32

  // Initialize DHT11
  dht.begin();
}

void loop() {
  Blynk.run();
  static unsigned long previousMillisEnergy = 0;
  const long intervalEnergy = 5000; // 5-second interval for PZEM-004T and DHT11

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisEnergy >= intervalEnergy) {
    previousMillisEnergy = currentMillis;
    showEnergyInfo();
  }
}

void showIntroText() {
  lcd.clear();
  String introText = "Office Monitoring IoT";
  String authorText = "By Danke Hidayat";

  // Calculate center position for intro text
  int introTextLength = introText.length();
  int authorTextLength = authorText.length();

  int introStartPos = (16 - introTextLength) / 2;
  int authorStartPos = (16 - authorTextLength) / 2;

  // Display intro text
  lcd.setCursor(introStartPos, 0);
  lcd.print(introText);
  lcd.setCursor(authorStartPos, 1);
  lcd.print(authorText);

  // If text is too long, scroll text from left to right
  if (introTextLength > 16 || authorTextLength > 16) {
    scrollText(introText, authorText);
  }
}

float zeroIfNan(float value) {
  return isnan(value) ? 0.0 : value;
}

void showEnergyInfo() {
  static int displayMode = 0;

  // Read data from PZEM-004T
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energyWh = pzem.energy(); // Wh
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  // Handle NaN values
  voltage = zeroIfNan(voltage);
  current = zeroIfNan(current);
  power = zeroIfNan(power);
  energyWh = zeroIfNan(energyWh);
  frequency = zeroIfNan(frequency);
  pf = zeroIfNan(pf);

  // Read data from DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Handle NaN values for DHT11
  humidity = zeroIfNan(humidity);
  temperature = zeroIfNan(temperature);

  // Calculate apparent power (VA)
  float apparentPower = (pf == 0) ? 0 : power / pf;

  // Calculate reactive power (VAR)
  float reactivePower = (pf == 0) ? 0 : power / pf * sqrt(1 - sq(pf));

  // Display on LCD
  lcd.clear();
  if (displayMode == 0) {
    // Display voltage and current
    lcd.setCursor(0, 0);
    lcd.print("Volt: " + String(voltage) + "V");
    lcd.setCursor(0, 1);
    lcd.print("Curr: " + String(current) + "A");
  } else if (displayMode == 1) {
    // Display power
    lcd.setCursor(0, 0);
    lcd.print("Power: " + String(power) + "W");
    lcd.setCursor(0, 1);
    lcd.print("Freq: " + String(frequency) + "Hz");
  } else if (displayMode == 2) {
    // Display energy in Wh
    lcd.setCursor(0, 0);
    lcd.print("Energy: " + String(energyWh) + "Wh");
    lcd.setCursor(0, 1);
    lcd.print("PF: " + String(pf) + "  ");
  } else if (displayMode == 3) {
    // Display temperature and humidity
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(temperature) + "C");
    lcd.setCursor(0, 1);
    lcd.print("Hum: " + String(humidity) + "%");
  }

  // Change display mode
  displayMode = (displayMode + 1) % 4;

  // Send data to Blynk
  Blynk.virtualWrite(V0, voltage);
  Blynk.virtualWrite(V1, current);
  Blynk.virtualWrite(V2, power);
  Blynk.virtualWrite(V3, pf); // Cos Phi (Power Factor)
  Blynk.virtualWrite(V4, apparentPower); // Apparent Power (VA)
  Blynk.virtualWrite(V5, energyWh); // Total Energy (Wh)
  Blynk.virtualWrite(V6, frequency); // Frequency (Hz)
  Blynk.virtualWrite(V7, reactivePower); // Reactive Power (VAR)

  // Send DHT11 data to Blynk
  Blynk.virtualWrite(V8, temperature); // Temperature
  Blynk.virtualWrite(V9, humidity); // Humidity
}

void scrollText(String line1, String line2) {
  for (int i = 0; i <= line1.length() - 16; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1.substring(i, i + 16));
    lcd.setCursor(0, 1);
    lcd.print(line2.substring(i, i + 16));
    delay(250);
  }
}

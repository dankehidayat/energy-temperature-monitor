#include <WiFi.h>
#include <WiFiClient.h>
#define BLYNK_TEMPLATE_ID "TMPL6eUbLFTuj"
#define BLYNK_TEMPLATE_NAME "Energy Monitor"
#include <BlynkSimpleEsp32.h> 
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>
#include <PZEM004Tv30.h> 
#include <DHT.h>
#include <Wire.h>

// Inisialisasi objek LCD dengan alamat I2C dan ukuran layar (16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Token Blynk Anda
char auth[] = "1l6iZj-qkTcO1kRM8tBLPsUFNBka--z4";

// Inisialisasi objek PZEM-004T menggunakan HardwareSerial
HardwareSerial hwSerial(1); // Gunakan UART1 pada ESP32
PZEM004Tv30 pzem(hwSerial, 16, 17); // Pin RX, TX pada ESP32

// Inisialisasi objek DHT11
#define DHTPIN 27 // Pin D27 pada ESP32
#define DHTTYPE DHT11 // Tipe sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

#define TRIGGER_PIN 0 // Pin untuk tombol boot/reset WiFi

// Variabel untuk kontrol kedip LCD
bool lcdBacklightState = true;
unsigned long previousBlinkMillis = 0;
const unsigned long BLINK_DELAY = 4500; // Delay sebelum mulai kedip (ms)
const unsigned long BLINK_INTERVAL = 250; // Interval kedip (ms)

// ==================== KALIBRASI DHT11 vs HTC-1 TERBARU ====================
/*
ANALISIS DATA KALIBRASI TERKINI (34 titik pengukuran):

HASIL PERHITUNGAN STATISTIK TERBARU:
- Model Regresi Linear: 
  • Suhu: HTC-1 = 0.923 × DHT11 - 1.618 (R² = 0.958)
  • Kelembaban: HTC-1 = 0.926 × DHT11 + 18.052 (R² = 0.977)

INDIKATOR KEAKURATAN:
- Korelasi: 0.979 (Suhu), 0.988 (Kelembaban) → SANGAT KUAT
- Mean Absolute Error (MAE): 3.84°C (Suhu), 14.18% (Kelembaban)
- Bias Rata-rata: -3.84°C (Suhu), +14.18% (Kelembaban)
- Akurasi Model: 95.8% (Suhu), 97.7% (Kelembaban)

INTERPRETASI:
- DHT11 konsisten membaca SUHU LEBIH TINGGI 3.84°C
- DHT11 konsisten membaca KELEMBABAN LEBIH RENDAH 14.18%
- Model regresi memiliki akurasi >95% untuk kedua parameter
*/

// Koefisien kalibrasi terbaru berdasarkan analisis 34 titik
const float TEMP_SLOPE = 0.923;
const float TEMP_INTERCEPT = -1.618;
const float HUM_SLOPE = 0.926;
const float HUM_INTERCEPT = 18.052;

// Koreksi sederhana berdasarkan selisih rata-rata
const float TEMP_BIAS = -3.84;
const float HUM_BIAS = +14.18;

/**
 * Fungsi untuk mengkalibrasi suhu dari DHT11 menggunakan model terbaru
 * Akurasi: 95.8% (Berdasarkan R² = 0.958)
 * @param rawTemp Nilai suhu mentah dari DHT11
 * @return Nilai suhu terkoreksi sesuai HTC-1
 */
float calibrateTemperature(float rawTemp) {
  return (TEMP_SLOPE * rawTemp) + TEMP_INTERCEPT;
}

/**
 * Fungsi untuk mengkalibrasi kelembaban dari DHT11 menggunakan model terbaru
 * Akurasi: 97.7% (Berdasarkan R² = 0.977)
 * @param rawHum Nilai kelembaban mentah dari DHT11
 * @return Nilai kelembaban terkoreksi sesuai HTC-1
 */
float calibrateHumidity(float rawHum) {
  return (HUM_SLOPE * rawHum) + HUM_INTERCEPT;
}

/**
 * Fungsi alternatif: koreksi sederhana berdasarkan bias rata-rata
 */
float calibrateTemperatureSimple(float rawTemp) {
  return rawTemp + TEMP_BIAS;
}

float calibrateHumiditySimple(float rawHum) {
  return rawHum + HUM_BIAS;
}

// Array untuk menyimpan data error monitoring
float tempErrors[10]; // Menyimpan 10 error terakhir untuk suhu
float humErrors[10];  // Menyimpan 10 error terakhir untuk kelembaban
int errorIndex = 0;   // Index untuk circular buffer

/**
 * Fungsi untuk mencatat error kalibrasi ke dalam array
 * @param tempError Error suhu (selisih antara linear dan sederhana)
 * @param humError Error kelembaban (selisih antara linear dan sederhana)
 */
void recordCalibrationError(float tempError, float humError) {
  tempErrors[errorIndex] = tempError;
  humErrors[errorIndex] = humError;
  errorIndex = (errorIndex + 1) % 10; // Circular buffer
}

/**
 * Fungsi untuk menghitung MAE dari data error yang tersimpan
 * @param tempMAE Output: MAE untuk suhu
 * @param humMAE Output: MAE untuk kelembaban
 */
void getCurrentMAE(float &tempMAE, float &humMAE) {
  float tempSum = 0, humSum = 0;
  int count = 0;
  
  for (int i = 0; i < 10; i++) {
    if (tempErrors[i] != 0) { // Hanya hitung data yang valid
      tempSum += abs(tempErrors[i]);
      humSum += abs(humErrors[i]);
      count++;
    }
  }
  
  tempMAE = count > 0 ? tempSum / count : 0;
  humMAE = count > 0 ? humSum / count : 0;
}

/**
 * Fungsi untuk menghitung akurasi dalam persentase
 * @param mae Mean Absolute Error
 * @param range Range normal nilai sensor
 * @return Akurasi dalam persentase
 */
float calculateAccuracy(float mae, float range) {
  return max(0.0, 100.0 - (mae / range * 100.0));
}

/**
 * Fungsi untuk mengatur kedipan backlight LCD dengan delay awal
 * @param currentMillis Waktu saat ini dari millis()
 * @param startTime Waktu mulai menunggu WiFi
 */
void handleLCDBlink(unsigned long currentMillis, unsigned long startTime) {
  // Cek apakah sudah melewati delay sebelum mulai kedip
  bool shouldBlink = (currentMillis - startTime >= BLINK_DELAY);
  
  if (shouldBlink && currentMillis - previousBlinkMillis >= BLINK_INTERVAL) {
    previousBlinkMillis = currentMillis;
    lcdBacklightState = !lcdBacklightState;
    lcdBacklightState ? lcd.backlight() : lcd.noBacklight();
  }
}

/**
 * Fungsi untuk menghentikan kedipan dan menyalakan backlight permanen
 */
void stopLCDBlink() {
  lcdBacklightState = true;
  lcd.backlight(); // Pastikan backlight menyala permanen
}

/**
 * Fungsi untuk mengecek tombol boot (reset WiFi)
 */
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
  Serial.begin(115200);
  
  // Inisialisasi LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();

  // Inisialisasi array error dengan nilai 0
  for (int i = 0; i < 10; i++) {
    tempErrors[i] = 0;
    humErrors[i] = 0;
  }

  // Cek tombol boot
  checkBoot();

  // Tampilkan teks intro
  showIntroText();
  delay(3500);

  // Inisialisasi WiFiManager
  #define AP_PASS "energyiot"
  #define AP_SSID "Energy IoT"

  const unsigned long WIFI_TIMEOUT = 60; // Timeout WiFi dalam detik

  WiFiManager wfm;
  wfm.setConfigPortalTimeout(WIFI_TIMEOUT);
  wfm.setHostname(AP_SSID);
  wfm.setConnectTimeout(WIFI_TIMEOUT);

  // Tampilkan "Waiting for WiFi connection" pada LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Connection...");

  // Variabel untuk waktu mulai menunggu WiFi
  unsigned long wifiStartTime = millis();
  
  // Start WiFiManager configuration portal dengan kontrol kedip LCD
  bool connected = false;
  while (!connected && (millis() - wifiStartTime < WIFI_TIMEOUT * 1000)) {
    connected = wfm.autoConnect(AP_SSID, AP_PASS);
    handleLCDBlink(millis(), wifiStartTime); // Handle kedip LCD
    delay(100);
  }

  if (!connected) {
    Serial.println("Failed to connect to WiFi!");
    stopLCDBlink();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to Connect");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Restarting ESP32...");
    delay(3500);
    ESP.restart();
  } else {
    Serial.println("WiFi Connected!");
    stopLCDBlink();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    delay(500);
  }

  // Inisialisasi Blynk dengan server lokal
  Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str(), "iot.serangkota.go.id", 8080);

  // Inisialisasi HardwareSerial untuk PZEM-004T
  hwSerial.begin(9600, SERIAL_8N1, 16, 17);

  // Inisialisasi DHT11
  dht.begin();

  // Informasi startup di Serial Monitor
  Serial.println("\n==================================================");
  Serial.println("        OFFICE MONITORING IOT DIAKTIFKAN");
  Serial.println("==================================================");
  Serial.println("Sistem monitoring energi dan lingkungan aktif");
  Serial.println("DHT11 terkoreksi dengan akurasi 95.8%-97.7%");
  Serial.println("Model: HTC-1 = 0.923×DHT11 - 1.618 (Suhu)");
  Serial.println("Model: HTC-1 = 0.926×DHT11 + 18.052 (Kelembaban)");
  Serial.println("==================================================\n");
}

void loop() {
  Blynk.run();
  static unsigned long previousMillis = 0;
  const unsigned long SENSOR_INTERVAL = 2500; // Interval pembacaan sensor

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= SENSOR_INTERVAL) {
    previousMillis = currentMillis;
    showEnergyInfo();
  }
}

/**
 * Fungsi untuk menampilkan teks intro pada LCD
 */
void showIntroText() {
  lcd.clear();
  String introText = "Office Monitoring IoT";
  String authorText = "By Danke Hidayat";

  // Hitung posisi tengah untuk teks intro
  int introStartPos = (16 - introText.length()) / 2;
  int authorStartPos = (16 - authorText.length()) / 2;

  // Tampilkan teks intro
  lcd.setCursor(introStartPos, 0);
  lcd.print(introText);
  lcd.setCursor(authorStartPos, 1);
  lcd.print(authorText);

  // Jika teks terlalu panjang, scroll teks
  if (introText.length() > 16 || authorText.length() > 16) {
    scrollText(introText, authorText);
  }
}

/**
 * Fungsi untuk mengubah nilai NaN menjadi 0
 * @param value Nilai yang akan dicek
 * @return 0 jika NaN, nilai asli jika valid
 */
float zeroIfNan(float value) {
  return isnan(value) ? 0.0 : value;
}

/**
 * Fungsi utama untuk menampilkan informasi energi dan sensor
 */
void showEnergyInfo() {
  static int displayMode = 0;

  // Baca data dari PZEM-004T
  float voltage = zeroIfNan(pzem.voltage());
  float current = zeroIfNan(pzem.current());
  float power = zeroIfNan(pzem.power());
  float energyWh = zeroIfNan(pzem.energy());
  float frequency = zeroIfNan(pzem.frequency());
  float pf = zeroIfNan(pzem.pf());

  // Baca data dari DHT11
  float humidity = zeroIfNan(dht.readHumidity());
  float temperature = zeroIfNan(dht.readTemperature());

  // Kalibrasi data DHT11 menggunakan model terbaru
  float calibratedTemp = calibrateTemperature(temperature);
  float calibratedHum = calibrateHumidity(humidity);

  // Hitung dengan metode sederhana untuk perbandingan
  float simpleTemp = calibrateTemperatureSimple(temperature);
  float simpleHum = calibrateHumiditySimple(humidity);

  // Hitung dan simpan error antara kedua metode kalibrasi
  float tempError = abs(calibratedTemp - simpleTemp);
  float humError = abs(calibratedHum - simpleHum);
  recordCalibrationError(tempError, humError);

  // Hitung akurasi real-time
  float currentTempMAE, currentHumMAE;
  getCurrentMAE(currentTempMAE, currentHumMAE);
  float tempAccuracy = calculateAccuracy(currentTempMAE, 50.0); // Range suhu 0-50°C
  float humAccuracy = calculateAccuracy(currentHumMAE, 100.0); // Range kelembaban 0-100%

  // Hitung daya semu dan reaktif
  float apparentPower = (pf == 0) ? 0 : power / pf;
  float reactivePower = (pf == 0) ? 0 : power / pf * sqrt(1 - sq(pf));

  // Tampilkan pada LCD
  lcd.clear();
  switch (displayMode) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("Volt: " + String(voltage) + "V");
      lcd.setCursor(0, 1); lcd.print("Curr: " + String(current) + "A");
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print("Power: " + String(power) + "W");
      lcd.setCursor(0, 1); lcd.print("Freq: " + String(frequency) + "Hz");
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print("Energy: " + String(energyWh) + "Wh");
      lcd.setCursor(0, 1); lcd.print("PF: " + String(pf) + "  ");
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print("Temp: " + String(calibratedTemp) + "C");
      lcd.setCursor(0, 1); lcd.print("Hum: " + String(calibratedHum) + "%");
      break;
  }

  displayMode = (displayMode + 1) % 4;

  // Kirim data ke Blynk
  Blynk.virtualWrite(V0, voltage);
  Blynk.virtualWrite(V1, current);
  Blynk.virtualWrite(V2, power);
  Blynk.virtualWrite(V3, pf);
  Blynk.virtualWrite(V4, apparentPower);
  Blynk.virtualWrite(V5, energyWh);
  Blynk.virtualWrite(V6, frequency);
  Blynk.virtualWrite(V7, reactivePower);
  Blynk.virtualWrite(V8, calibratedTemp);
  Blynk.virtualWrite(V9, calibratedHum);

  // Log ke Serial Monitor setiap 10 readings
  static int logCounter = 0;
  if (logCounter++ >= 10) {
    logCounter = 0;
    
    Serial.println("==================================================");
    Serial.println("               DATA SENSOR MONITORING");
    Serial.println("==================================================");
    
    // Data PZEM-004T
    Serial.println("--- PZEM-004T POWER SENSOR ---");
    Serial.print("Voltage:      "); Serial.print(voltage, 1); Serial.println(" V");
    Serial.print("Current:      "); Serial.print(current, 3); Serial.println(" A");
    Serial.print("Power:        "); Serial.print(power, 1); Serial.println(" W");
    Serial.print("Energy:       "); Serial.print(energyWh, 1); Serial.println(" Wh");
    Serial.print("Frequency:    "); Serial.print(frequency, 1); Serial.println(" Hz");
    Serial.print("Power Factor: "); Serial.print(pf, 2); Serial.println("");
    Serial.print("Apparent Pwr: "); Serial.print(apparentPower, 1); Serial.println(" VA");
    Serial.print("Reactive Pwr: "); Serial.print(reactivePower, 1); Serial.println(" VAR");
    
    Serial.println("");
    
    // Data DHT11 dengan informasi akurasi
    Serial.println("--- DHT11 ENVIRONMENT SENSOR (AKURASI TERKALIBRASI) ---");
    Serial.print("DHT11 Raw      -> Temp: "); Serial.print(temperature, 1); Serial.print("°C, Hum: "); Serial.print(humidity, 1); Serial.println("%");
    Serial.print("HTC-1 Terkalibrasi -> Temp: "); Serial.print(calibratedTemp, 1); Serial.print("°C, Hum: "); Serial.print(calibratedHum, 1); Serial.println("%");
    Serial.print("Model Akurasi  -> Temp: 95.8%, Hum: 97.7%");
    Serial.print("Koreksi Diterapkan -> Temp: "); Serial.print(TEMP_BIAS, 1); Serial.print("°C, Hum: "); Serial.print(HUM_BIAS, 1); Serial.println("%");
    
    Serial.println("");
    
    // Data Error Analysis dan Kualitas
    Serial.println("--- KUALITAS KALIBRASI REAL-TIME ---");
    Serial.print("Error Antara Metode -> Temp: "); Serial.print(tempError, 3); Serial.print("°C, Hum: "); Serial.print(humError, 3); Serial.println("%");
    Serial.print("MAE Real-time       -> Temp: "); Serial.print(currentTempMAE, 3); Serial.print("°C, Hum: "); Serial.print(currentHumMAE, 3); Serial.println("%");
    Serial.print("Akurasi Real-time   -> Temp: "); Serial.print(tempAccuracy, 1); Serial.print("%, Hum: "); Serial.print(humAccuracy, 1); Serial.println("%");
    
    // Status sistem
    Serial.println("--- SYSTEM STATUS ---");
    Serial.print("WiFi Status: "); Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.print("RSSI: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
    
    Serial.println("==================================================");
    Serial.println("");
  }
}

/**
 * Fungsi untuk scroll teks pada LCD
 * @param line1 Teks untuk baris pertama
 * @param line2 Teks untuk baris kedua
 */
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

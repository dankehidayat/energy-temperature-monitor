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

// ==================== HARDWARE INITIALIZATION ====================

// Inisialisasi objek LCD dengan alamat I2C dan ukuran layar (16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Token Blynk Anda
char auth[] = "";

// Inisialisasi objek PZEM-004T menggunakan HardwareSerial
HardwareSerial hwSerial(1); // Gunakan UART1 pada ESP32
PZEM004Tv30 pzem(hwSerial, 16, 17); // Pin RX, TX pada ESP32

// Inisialisasi objek DHT11
#define DHTPIN 27 // Pin D27 pada ESP32
#define DHTTYPE DHT11 // Tipe sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

#define TRIGGER_PIN 0 // Pin untuk tombol boot/reset WiFi

// ==================== SYSTEM VARIABLES ====================

// Variabel untuk kontrol kedip LCD
bool lcdBacklightState = true;
unsigned long previousBlinkMillis = 0;
const unsigned long BLINK_DELAY = 4500; // Delay sebelum mulai kedip (ms)
const unsigned long BLINK_INTERVAL = 250; // Interval kedip (ms)

// ==================== TEMPERATURE SYSTEM ====================

/*
ANALISIS DATA KALIBRASI TERKINI (34 titik pengukuran):
- Model Regresi Linear: 
  • Suhu: HTC-1 = 0.923 × DHT11 - 1.618 (R² = 0.958)
  • Kelembaban: HTC-1 = 0.926 × DHT11 + 18.052 (R² = 0.977)

METODOLOGI REGRESI LINEAR:
- R² (R-squared) = 0.958 untuk suhu, 0.977 untuk kelembaban
- Mengindikasikan model menjelaskan 95.8% variasi data suhu dan 97.7% variasi data kelembaban
- Model: y = mx + c, dimana y = nilai terkoreksi, x = nilai mentah DHT11
- m = slope (kemiringan garis), c = intercept (titik potong sumbu y)

INTERPRETASI STATISTIK:
- Korelasi: 0.979 (Suhu), 0.988 (Kelembaban) → SANGAT KUAT
- Mean Absolute Error (MAE): 3.84°C (Suhu), 14.18% (Kelembaban)
- Akurasi Model: 95.8% (Suhu), 97.7% (Kelembaban)
*/

// Koefisien regresi linear untuk kalibrasi suhu
const float TEMP_SLOPE = 0.923;      // Slope garis regresi (perubahan HTC-1 per unit DHT11)
const float TEMP_INTERCEPT = -1.618; // Intercept garis regresi (nilai HTC-1 ketika DHT11 = 0)

// Koefisien regresi linear untuk kalibrasi kelembaban  
const float HUM_SLOPE = 0.926;       // Slope garis regresi
const float HUM_INTERCEPT = 18.052;  // Intercept garis regresi

// Koreksi sederhana berdasarkan selisih rata-rata (bias)
const float TEMP_BIAS = -3.84; // DHT11 membaca 3.84°C lebih tinggi dari HTC-1
const float HUM_BIAS = +14.18; // DHT11 membaca 14.18% lebih rendah dari HTC-1

// Array untuk menyimpan data error monitoring
float tempErrors[10]; // Menyimpan 10 error terakhir untuk suhu
float humErrors[10];  // Menyimpan 10 error terakhir untuk kelembaban
int errorIndex = 0;   // Index untuk circular buffer

/**
 * TEMPERATURE: Kalibrasi suhu menggunakan model regresi linear
 * Persamaan: HTC-1 = 0.923 × DHT11 - 1.618
 * @param rawTemp Nilai suhu mentah dari DHT11 dalam °C
 * @return Nilai suhu terkoreksi sesuai standar HTC-1 dalam °C
 * @note Akurasi: 95.8% berdasarkan R² = 0.958 dari 34 titik pengukuran
 */
float calibrateTemperature(float rawTemp) {
  return (TEMP_SLOPE * rawTemp) + TEMP_INTERCEPT;
}

/**
 * TEMPERATURE: Kalibrasi kelembaban menggunakan model regresi linear  
 * Persamaan: HTC-1 = 0.926 × DHT11 + 18.052
 * @param rawHum Nilai kelembaban mentah dari DHT11 dalam %
 * @return Nilai kelembaban terkoreksi sesuai standar HTC-1 dalam %
 * @note Akurasi: 97.7% berdasarkan R² = 0.977 dari 34 titik pengukuran
 */
float calibrateHumidity(float rawHum) {
  return (HUM_SLOPE * rawHum) + HUM_INTERCEPT;
}

/**
 * TEMPERATURE: Kalibrasi sederhana berdasarkan bias rata-rata
 * Metode alternatif menggunakan koreksi konstan (bias)
 * @param rawTemp Nilai suhu mentah dari DHT11 dalam °C  
 * @return Nilai suhu terkoreksi (rawTemp - 3.84°C)
 */
float calibrateTemperatureSimple(float rawTemp) {
  return rawTemp + TEMP_BIAS;
}

/**
 * TEMPERATURE: Kalibrasi sederhana berdasarkan bias rata-rata
 * @param rawHum Nilai kelembaban mentah dari DHT11 dalam %
 * @return Nilai kelembaban terkoreksi (rawHum + 14.18%)
 */
float calibrateHumiditySimple(float rawHum) {
  return rawHum + HUM_BIAS;
}

/**
 * TEMPERATURE: Mencatat error kalibrasi untuk analisis akurasi
 * Menggunakan circular buffer untuk menyimpan 10 error terakhir
 * @param tempError Selisih antara metode regresi linear dan metode sederhana untuk suhu
 * @param humError Selisih antara metode regresi linear dan metode sederhana untuk kelembaban
 */
void recordCalibrationError(float tempError, float humError) {
  tempErrors[errorIndex] = tempError;
  humErrors[errorIndex] = humError;
  errorIndex = (errorIndex + 1) % 10; // Circular buffer - kembali ke 0 setelah index 9
}

/**
 * TEMPERATURE: Menghitung Mean Absolute Error (MAE) real-time
 * MAE = rata-rata absolute error dari data yang tersimpan
 * @param tempMAE Output: MAE untuk suhu dalam °C
 * @param humMAE Output: MAE untuk kelembaban dalam %
 */
void getCurrentMAE(float &tempMAE, float &humMAE) {
  float tempSum = 0, humSum = 0;
  int count = 0;
  
  for (int i = 0; i < 10; i++) {
    if (tempErrors[i] != 0) { // Hanya hitung data yang valid (bukan 0)
      tempSum += abs(tempErrors[i]);
      humSum += abs(humErrors[i]);
      count++;
    }
  }
  
  tempMAE = count > 0 ? tempSum / count : 0;
  humMAE = count > 0 ? humSum / count : 0;
}

/**
 * TEMPERATURE: Menghitung akurasi dalam persentase
 * Akurasi = 100% - (MAE / Range × 100%)
 * @param mae Mean Absolute Error dari pengukuran
 * @param Range normal nilai sensor (50°C untuk suhu, 100% untuk kelembaban)
 * @return Akurasi dalam persentase (0-100%)
 */
float calculateAccuracy(float mae, float range) {
  return max(0.0, 100.0 - (mae / range * 100.0));
}

// ==================== ENERGY SYSTEM ====================

/**
 * ENERGY: Mengubah nilai NaN (Not a Number) menjadi 0
 * @param value Nilai yang akan dicek dari sensor PZEM-004T
 * @return 0 jika NaN, nilai asli jika valid
 */
float zeroIfNan(float value) {
  return isnan(value) ? 0.0 : value;
}

// ==================== FUZZY LOGIC SYSTEM ====================

/**
 * FUZZY: Sistem logika fuzzy untuk klasifikasi kenyamanan termal
 * Berdasarkan standar ASHRAE 55 dan ISO 7730 untuk kenyamanan termal
 * 
 * BASIS PENGETAHUAN:
 * - Suhu Dingin (COLD): 16-22°C (di bawah standar kenyamanan)
 * - Suhu Nyaman (COMFORTABLE): 20-26°C (range optimal kantor)  
 * - Suhu Hangat (WARM): 24-30°C (mendekati ketidaknyamanan)
 * - Suhu Panas (HOT): 28-34°C (di atas batas kenyamanan)
 * 
 * RULE BASE (8 aturan):
 * 1. IF Suhu is Dingin THEN Kenyamanan is Dingin
 * 2. IF Suhu is Nyaman AND Kelembaban is Nyaman THEN Kenyamanan is Nyaman
 * 3. IF Suhu is Nyaman AND Kelembaban is Kering THEN Kenyamanan is Sejuk  
 * 4. IF Suhu is Nyaman AND Kelembaban is Lembab THEN Kenyamanan is Hangat
 * 5. IF Suhu is Hangat THEN Kenyamanan is Hangat
 * 6. IF Suhu is Panas THEN Kenyamanan is Panas
 * 7. IF Suhu is Dingin AND Kelembaban is Lembab THEN Kenyamanan is Sejuk (terasa lebih dingin)
 * 8. IF Suhu is Panas AND Kelembaban is Lembab THEN Kenyamanan is Panas (terasa lebih panas)
 * 
 * @param temp Suhu terkoreksi dalam °C
 * @param humidity Kelembaban terkoreksi dalam %
 * @return Kategori kenyamanan: COLD, COOL, COMFORTABLE, WARM, HOT
 */
String fuzzyTemperatureComfort(float temp, float humidity) {
  // Fuzzyfikasi suhu dengan fungsi keanggotaan trapezoidal
  float cold = (temp <= 18) ? 1.0 : (temp <= 22) ? (22 - temp) / 4.0 : 0.0;
  float comfortable = (temp >= 20 && temp <= 23) ? (temp - 20) / 3.0 : 
                     (temp > 23 && temp <= 26) ? (26 - temp) / 3.0 : 0.0;
  float warm = (temp >= 24 && temp <= 27) ? (temp - 24) / 3.0 :
               (temp > 27 && temp <= 30) ? (30 - temp) / 3.0 : 0.0;
  float hot = (temp >= 28) ? 1.0 : (temp >= 26) ? (temp - 26) / 2.0 : 0.0;

  // Fuzzyfikasi kelembaban 
  float dry = (humidity <= 30) ? 1.0 : (humidity <= 40) ? (40 - humidity) / 10.0 : 0.0;
  float comfortable_hum = (humidity >= 30 && humidity <= 50) ? (humidity - 30) / 20.0 :
                         (humidity > 50 && humidity <= 70) ? (70 - humidity) / 20.0 : 0.0;
  float humid = (humidity >= 60) ? 1.0 : (humidity >= 50) ? (humidity - 50) / 10.0 : 0.0;

  // Aplikasi 8 rules berdasarkan penelitian kenyamanan termal
  float cold_strength = cold;
  float cool_strength = max(min(comfortable, dry), min(cold, humid));
  float comfortable_strength = min(comfortable, comfortable_hum);
  float warm_strength = max(warm, min(comfortable, humid));
  float hot_strength = max(hot, min(hot, humid));

  // Defuzzifikasi - metode max membership
  float strengths[] = {cold_strength, cool_strength, comfortable_strength, warm_strength, hot_strength};
  String categories[] = {"COLD", "COOL", "COMFORTABLE", "WARM", "HOT"};
  
  int max_index = 0;
  for (int i = 1; i < 5; i++) {
    if (strengths[i] > strengths[max_index]) {
      max_index = i;
    }
  }
  
  return categories[max_index];
}

/**
 * FUZZY: Sistem logika fuzzy untuk klasifikasi konsumsi energi
 * Berdasarkan standar IEEE 1159 untuk kualitas daya dan efisiensi energi
 * 
 * BASIS PENGETAHUAN:
 * VOLTAGE (Tegangan) - Standar PLN: 220V ±10%:
 * - Rendah (LOW): <210V (risiko brownout)
 * - Normal (NORMAL): 210-230V (range optimal)
 * - Tinggi (HIGH): >230V (risiko kerusakan equipment)
 * 
 * POWER (Daya) - Konsumsi kantor kecil:
 * - Ekonomis (ECONOMICAL): <300W (konsumsi rendah)
 * - Normal (NORMAL): 200-1500W (konsumsi standar)  
 * - Boros (WASTEFUL): >1000W (konsumsi tinggi)
 * 
 * POWER FACTOR (Faktor Daya) - Standar industri:
 * - Buruk (POOR): <0.7 (efisiensi buruk)
 * - Cukup (FAIR): 0.65-0.85 (efisiensi cukup)
 * - Baik (GOOD): >0.8 (efisiensi baik)
 * 
 * REACTIVE POWER (Daya Reaktif) - Indikator kualitas daya:
 * - Rendah (LOW): <500VAR (beban reaktif rendah)
 * - Sedang (MEDIUM): 400-1500VAR (beban reaktif sedang)
 * - Tinggi (HIGH): >1000VAR (beban reaktif tinggi)
 * 
 * RULE BASE (13 aturan) - Berdasarkan best practice manajemen energi:
 * GROUP 1: Kondisi ideal efisiensi tinggi → ECONOMICAL
 * GROUP 2: Operasi normal standar → NORMAL  
 * GROUP 3: Kondisi bermasalah perlu perhatian → WASTEFUL
 * 
 * @param voltage Tegangan dalam Volt
 * @param power Daya aktif dalam Watt
 * @param powerFactor Faktor daya (0-1)
 * @param reactivePower Daya reaktif dalam VAR
 * @return Kategori konsumsi energi: ECONOMICAL, NORMAL, WASTEFUL
 */
String fuzzyEnergyConsumption(float voltage, float power, float powerFactor, float reactivePower) {
  // Fuzzyfikasi voltage (standar PLN: 220V ±10%)
  float v_low = (voltage <= 200) ? 1.0 : (voltage <= 210) ? (210 - voltage) / 10.0 : 0.0;
  float v_normal = (voltage >= 210 && voltage <= 220) ? (voltage - 210) / 10.0 :
                   (voltage > 220 && voltage <= 230) ? (230 - voltage) / 10.0 : 0.0;
  float v_high = (voltage >= 230) ? 1.0 : (voltage >= 220) ? (voltage - 220) / 10.0 : 0.0;

  // Fuzzyfikasi power (konsumsi kantor kecil: 200-1500W)
  float p_economical = (power <= 200) ? 1.0 : (power <= 300) ? (300 - power) / 100.0 : 0.0;
  float p_normal = (power >= 200 && power <= 750) ? (power - 200) / 550.0 :
                   (power > 750 && power <= 1500) ? (1500 - power) / 750.0 : 0.0;
  float p_wasteful = (power >= 1000) ? 1.0 : (power >= 750) ? (power - 750) / 250.0 : 0.0;

  // Fuzzyfikasi power factor (standar industri: >0.8 baik)
  float pf_poor = (powerFactor <= 0.65) ? 1.0 : (powerFactor <= 0.7) ? (0.7 - powerFactor) / 0.05 : 0.0;
  float pf_fair = (powerFactor >= 0.65 && powerFactor <= 0.75) ? (powerFactor - 0.65) / 0.1 :
                  (powerFactor > 0.75 && powerFactor <= 0.85) ? (0.85 - powerFactor) / 0.1 : 0.0;
  float pf_good = (powerFactor >= 0.8) ? 1.0 : (powerFactor >= 0.75) ? (powerFactor - 0.75) / 0.05 : 0.0;

  // Fuzzyfikasi reactive power (indikator kualitas daya)
  float r_low = (reactivePower <= 400) ? 1.0 : (reactivePower <= 500) ? (500 - reactivePower) / 100.0 : 0.0;
  float r_medium = (reactivePower >= 400 && reactivePower <= 950) ? (reactivePower - 400) / 550.0 :
                   (reactivePower > 950 && reactivePower <= 1500) ? (1500 - reactivePower) / 550.0 : 0.0;
  float r_high = (reactivePower >= 1000) ? 1.0 : (reactivePower >= 950) ? (reactivePower - 950) / 50.0 : 0.0;

  // Rule Base - 13 rules berdasarkan best practice manajemen energi
  float economical_strength = 0.0;
  float normal_strength = 0.0;
  float wasteful_strength = 0.0;

  // GROUP 1: Kondisi ideal efisiensi tinggi → ECONOMICAL
  economical_strength = max(economical_strength, min(p_economical, pf_good));
  economical_strength = max(economical_strength, min(p_economical, v_normal));
  economical_strength = max(economical_strength, min(p_economical, r_low));

  // GROUP 2: Operasi normal standar → NORMAL
  normal_strength = max(normal_strength, min(p_normal, pf_fair));
  normal_strength = max(normal_strength, min(p_normal, v_normal));
  normal_strength = max(normal_strength, min(p_normal, r_medium));

  // GROUP 3: Kondisi bermasalah perlu perhatian → WASTEFUL
  wasteful_strength = max(wasteful_strength, p_wasteful);
  wasteful_strength = max(wasteful_strength, pf_poor);
  wasteful_strength = max(wasteful_strength, v_low);
  wasteful_strength = max(wasteful_strength, v_high);
  wasteful_strength = max(wasteful_strength, r_high);
  wasteful_strength = max(wasteful_strength, min(p_normal, pf_poor));
  wasteful_strength = max(wasteful_strength, min(p_wasteful, r_high));

  // Defuzzifikasi dengan weighted decision
  if (economical_strength > normal_strength && economical_strength > wasteful_strength) {
    return "ECONOMICAL";
  } else if (wasteful_strength > normal_strength && wasteful_strength > economical_strength) {
    return "WASTEFUL";
  } else {
    return "NORMAL";
  }
}

/**
 * FUZZY: Update Blynk dengan status fuzzy terpisah
 * V10 = Temperature Comfort Status
 * V11 = Energy Consumption Status
 */
void updateBlynkFuzzyStatus(float temperature, float humidity, float voltage, float power, float powerFactor, float reactivePower) {
  String tempComfort = fuzzyTemperatureComfort(temperature, humidity);
  String energyStatus = fuzzyEnergyConsumption(voltage, power, powerFactor, reactivePower);
  
  // Kirim ke virtual pins terpisah untuk Google Sheets
  Blynk.virtualWrite(V10, tempComfort);
  Blynk.virtualWrite(V11, energyStatus);
  
  // Debug information ke Serial Monitor
  Serial.println("=== SMART OFFICE GUARDIAN - FUZZY STATUS ===");
  Serial.print("Thermal Comfort: "); Serial.print(tempComfort);
  Serial.print(" | Temperature: "); Serial.print(temperature, 1); Serial.print("°C");
  Serial.print(" | Humidity: "); Serial.print(humidity, 1); Serial.println("%");
  
  Serial.print("Energy Status: "); Serial.print(energyStatus);
  Serial.print(" | Voltage: "); Serial.print(voltage, 1); Serial.print("V");
  Serial.print(" | Power: "); Serial.print(power, 1); Serial.print("W");
  Serial.print(" | PF: "); Serial.print(powerFactor, 2);
  Serial.print(" | Reactive: "); Serial.print(reactivePower, 1); Serial.println("VAR");
  Serial.println();
}

// ==================== SYSTEM CONTROL FUNCTIONS ====================

/**
 * SYSTEM: Mengatur kedipan backlight LCD dengan delay awal
 * Memberikan feedback visual selama proses koneksi WiFi
 * @param currentMillis Waktu saat ini dari millis() dalam milidetik
 * @param startTime Waktu mulai menunggu WiFi dalam milidetik
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
 * SYSTEM: Menghentikan kedipan dan menyalakan backlight permanen
 * Dipanggil setelah koneksi WiFi berhasil
 */
void stopLCDBlink() {
  lcdBacklightState = true;
  lcd.backlight(); // Pastikan backlight menyala permanen
}

/**
 * SYSTEM: Mengecek tombol boot untuk reset WiFi
 * Jika tombol ditekan >5 detik, reset konfigurasi WiFi dan restart
 */
void checkBoot() {
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  if (digitalRead(TRIGGER_PIN) == LOW) {
    delay(100);
    if (digitalRead(TRIGGER_PIN) == LOW) {
      Serial.println("Boot button pressed");
      delay(5000);
      if (digitalRead(TRIGGER_PIN) == LOW) {
        Serial.println("Boot button held - Erasing WiFi config and restarting");
        WiFiManager wfm;
        wfm.resetSettings();
        ESP.restart();
      }
    }
  }
}

/**
 * SYSTEM: Menampilkan teks intro pada LCD
 */
void showIntroText() {
  lcd.clear();
  String introText = "EcoOffice";
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
 * SYSTEM: Fungsi untuk scroll teks pada LCD
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

// ==================== MAIN SYSTEM FUNCTIONS ====================

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
  #define AP_PASS "guard14n0ff1ce"
  #define AP_SSID "EcoOffice"

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
    
    // Display IP Address after WiFi connection
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    
    String ipAddress = WiFi.localIP().toString();
    if (ipAddress.length() <= 16) {
      lcd.print("IP:" + ipAddress);
    } else {
      lcd.print("IP:" + ipAddress.substring(0, 13) + "...");
    }
    delay(3000);
  }

  // Inisialisasi Blynk dengan server lokal
  Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str(), "iot.serangkota.go.id", 8080);

  // Inisialisasi HardwareSerial untuk PZEM-004T
  hwSerial.begin(9600, SERIAL_8N1, 16, 17);

  // Inisialisasi DHT11
  dht.begin();

  // Tampilkan system ready
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  lcd.setCursor(0, 1);
  lcd.print("Monitoring...");
  delay(1500);

  // Informasi startup di Serial Monitor
  Serial.println("\n==================================================");
  Serial.println("        OFFICE MONITORING IOT DIAKTIFKAN");
  Serial.println("==================================================");
  Serial.println("Sistem monitoring energi dan lingkungan aktif");
  Serial.println("DHT11 terkoreksi dengan akurasi 95.8%-97.7%");
  Serial.println("Model: HTC-1 = 0.923×DHT11 - 1.618 (Suhu)");
  Serial.println("Model: HTC-1 = 0.926×DHT11 + 18.052 (Kelembaban)");
  Serial.println("IP Address: " + WiFi.localIP().toString());
  Serial.println("==================================================\n");
}

void loop() {
  Blynk.run();
  static unsigned long previousMillis = 0;
  const unsigned long SENSOR_INTERVAL = 3000; // Interval pembacaan sensor 3 detik

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= SENSOR_INTERVAL) {
    previousMillis = currentMillis;
    showEnergyInfo();
  }
}

/**
 * SYSTEM: Fungsi utama untuk menampilkan informasi energi dan sensor
 */
void showEnergyInfo() {
  static int displayMode = 0;

  // ENERGY: Baca data dari PZEM-004T
  float voltage = zeroIfNan(pzem.voltage());
  float current = zeroIfNan(pzem.current());
  float power = zeroIfNan(pzem.power());
  float energyWh = zeroIfNan(pzem.energy());
  float frequency = zeroIfNan(pzem.frequency());
  float pf = zeroIfNan(pzem.pf());

  // TEMPERATURE: Baca data dari DHT11
  float humidity = zeroIfNan(dht.readHumidity());
  float temperature = zeroIfNan(dht.readTemperature());

  // TEMPERATURE: Kalibrasi data DHT11 menggunakan model terbaru
  float calibratedTemp = calibrateTemperature(temperature);
  float calibratedHum = calibrateHumidity(humidity);

  // TEMPERATURE: Hitung dengan metode sederhana untuk perbandingan
  float simpleTemp = calibrateTemperatureSimple(temperature);
  float simpleHum = calibrateHumiditySimple(humidity);

  // TEMPERATURE: Hitung dan simpan error antara kedua metode kalibrasi
  float tempError = abs(calibratedTemp - simpleTemp);
  float humError = abs(calibratedHum - simpleHum);
  recordCalibrationError(tempError, humError);

  // ENERGY: Hitung daya semu dan reaktif
  float apparentPower = (pf == 0) ? 0 : power / pf;
  float reactivePower = (pf == 0) ? 0 : sqrt(sq(apparentPower) - sq(power));

  // FIXED: Call fuzzy logic function to update V10 and V11
  updateBlynkFuzzyStatus(calibratedTemp, calibratedHum, voltage, power, pf, reactivePower);

  // Tampilkan pada LCD dengan 5 display modes terpisah
  lcd.clear();
  switch (displayMode) {
    case 0:
      // Mode 1: Data Tegangan dan Arus
      lcd.setCursor(0, 0); 
      lcd.print("Voltage: " + String(voltage, 1) + "V");
      lcd.setCursor(0, 1);
      lcd.print("Current: " + String(current, 3) + "A");
      break;
      
    case 1:
      // Mode 2: Data Daya dan Frekuensi
      lcd.setCursor(0, 0); 
      lcd.print("Power: " + String(power, 1) + "W");
      lcd.setCursor(0, 1);
      lcd.print("Freq: " + String(frequency, 1) + "Hz");
      break;
      
    case 2:
      // Mode 3: Data Energi dan Faktor Daya
      lcd.setCursor(0, 0); 
      lcd.print("Energy: " + String(energyWh, 1) + "Wh");
      lcd.setCursor(0, 1);
      lcd.print("PF: " + String(pf, 2));
      break;
      
    case 3:
      // Mode 4: Data Suhu dan Kelembaban
      lcd.setCursor(0, 0); 
      lcd.print("Temp: " + String(calibratedTemp, 1) + "C");
      lcd.setCursor(0, 1);
      lcd.print("Humidity: " + String(calibratedHum, 1) + "%");
      break;
      
    case 4:
      // Mode 5: Status Fuzzy Logic
      {
        String tempComfort = fuzzyTemperatureComfort(calibratedTemp, calibratedHum);
        String energyStatus = fuzzyEnergyConsumption(voltage, power, pf, reactivePower);
        lcd.setCursor(0, 0); 
        lcd.print("Comfort: " + tempComfort);
        lcd.setCursor(0, 1);
        lcd.print("Energy: " + energyStatus);
      }
      break;
  }

  displayMode = (displayMode + 1) % 5;

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

  // Log ke Serial Monitor setiap 6 readings (18 detik)
  static int logCounter = 0;
  if (logCounter++ >= 6) {
    logCounter = 0;
    
    // TEMPERATURE: Hitung akurasi real-time
    float currentTempMAE, currentHumMAE;
    getCurrentMAE(currentTempMAE, currentHumMAE);
    float tempAccuracy = calculateAccuracy(currentTempMAE, 50.0); // Range suhu 0-50°C
    float humAccuracy = calculateAccuracy(currentHumMAE, 100.0); // Range kelembaban 0-100%
    
    Serial.println("================ SENSOR DATA ================");
    Serial.println("Power Data:");
    Serial.println("  Voltage: " + String(voltage, 1) + "V | Current: " + String(current, 3) + "A");
    Serial.println("  Power: " + String(power, 1) + "W | Energy: " + String(energyWh, 1) + "Wh");
    Serial.println("  Power Factor: " + String(pf, 2) + " | Frequency: " + String(frequency, 1) + "Hz");
    
    Serial.println("Environment Data:");
    Serial.println("  Temperature: " + String(calibratedTemp, 1) + "°C (Raw: " + String(temperature, 1) + "°C)");
    Serial.println("  Humidity: " + String(calibratedHum, 1) + "% (Raw: " + String(humidity, 1) + "%)");
    Serial.println("  Calibration Accuracy - Temp: " + String(tempAccuracy, 1) + "%, Hum: " + String(humAccuracy, 1) + "%");
    
    // Show fuzzy classification in serial monitor
    String tempComfort = fuzzyTemperatureComfort(calibratedTemp, calibratedHum);
    String energyStatus = fuzzyEnergyConsumption(voltage, power, pf, reactivePower);
    Serial.println("Fuzzy Classification:");
    Serial.println("  Thermal Comfort: " + tempComfort);
    Serial.println("  Energy Status: " + energyStatus);
    Serial.println("============================================\n");
  }
}

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
char auth[] = "insertyourtokenhere";

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
const long blinkInterval = 250; // Interval kedip 250ms

// ==================== KALIBRASI DHT11 vs HTC-1 ====================
/*
ANALISIS DATA KALIBRASI (34 titik pengukuran):

HASIL PERHITUNGAN ULANG:
- Rata-rata DHT11: 28.9°C, HTC-1: 24.8°C -> SELISIH: 4.1°C
- Rata-rata DHT11: 52.4%, HTC-1: 66.2% -> SELISIH: 13.8%

HASIL REGRESI LINEAR:
- Suhu: HTC-1 = 0.845 × DHT11 + 0.642 (R² = 0.897)
- Kelembaban: HTC-1 = 1.135 × DHT11 + 6.732 (R² = 0.823)

MEAN ABSOLUTE ERROR (MAE):
- Suhu: 0.42°C
- Kelembaban: 2.87%

KOREKSI YANG DITERAPKAN:
- Suhu: DHT11 - 4.1°C
- Kelembaban: DHT11 + 13.8%
*/

/**
 * Fungsi untuk mengkalibrasi suhu dari DHT11
 * @param rawTemp Nilai suhu mentah dari DHT11
 * @return Nilai suhu terkoreksi sesuai HTC-1
 */
float calibrateTemperature(float rawTemp) {
  // Koreksi: DHT11 membaca 4.1°C LEBIH TINGGI dari HTC-1
  return rawTemp - 4.1;
}

/**
 * Fungsi untuk mengkalibrasi kelembaban dari DHT11
 * @param rawHum Nilai kelembaban mentah dari DHT11
 * @return Nilai kelembaban terkoreksi sesuai HTC-1
 */
float calibrateHumidity(float rawHum) {
  // Koreksi: DHT11 membaca 13.8% LEBIH RENDAH dari HTC-1
  return rawHum + 13.8;
}

// Fungsi kalibrasi linear (untuk referensi - tidak digunakan utama)
float calibrateTemperatureLinear(float rawTemp) {
  return 0.845 * rawTemp + 0.642;
}

float calibrateHumidityLinear(float rawHum) {
  return 1.135 * rawHum + 6.732;
}

/**
 * Fungsi untuk mengatur kedipan backlight LCD
 * @param currentMillis Waktu saat ini dari millis()
 */
void handleLCDBlink(unsigned long currentMillis) {
  if (currentMillis - previousBlinkMillis >= blinkInterval) {
    previousBlinkMillis = currentMillis;
    lcdBacklightState = !lcdBacklightState;
    if (lcdBacklightState) {
      lcd.backlight(); // Nyalakan backlight
    } else {
      lcd.noBacklight(); // Matikan backlight
    }
  }
}

/**
 * Fungsi untuk menghentikan kedipan dan menyalakan backlight permanen
 */
void stopLCDBlink() {
  lcdBacklightState = true;
  lcd.backlight(); // Pastikan backlight menyala
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

  // Cek tombol boot
  checkBoot();

  // Tampilkan teks intro
  showIntroText();
  delay(3500);

  // Inisialisasi WiFiManager
  #define AP_PASS "energyiot" // Password untuk SSID ESP32
  #define AP_SSID "Energy IoT" // Nama SSID ESP32

  // Variabel timeout
  const unsigned long Timeout = 60;

  WiFiManager wfm;
  wfm.setConfigPortalTimeout(Timeout); // Timeout setelah 60 detik
  wfm.setHostname(AP_SSID);
  wfm.setConnectTimeout(Timeout);

  // Tampilkan "Waiting for WiFi connection" pada LCD dengan kedip
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Connection...");

  // Variabel untuk waktu mulai menunggu WiFi
  unsigned long wifiStartTime = millis();
  
  // Start WiFiManager configuration portal dengan kedip LCD
  bool connected = false;
  while (!connected && (millis() - wifiStartTime < Timeout * 1000)) {
    connected = wfm.autoConnect(AP_SSID, AP_PASS);
    handleLCDBlink(millis()); // Kedipkan LCD selama menunggu
    delay(100);
  }

  if (!connected) {
    Serial.println("Failed to connect to WiFi!");
    stopLCDBlink(); // Hentikan kedip
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to Connect");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Restarting ESP32...");
    delay(3500); // Tampilkan pesan selama 3.5 detik
    ESP.restart(); // Restart ESP32 untuk mencoba lagi
  } else {
    Serial.println("WiFi Connected!");
    stopLCDBlink(); // Hentikan kedip dan nyalakan backlight permanen
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    delay(500); // Tampilkan pesan hanya 500ms sebelum ke sensor
  }

  // Inisialisasi Blynk dengan server lokal di Kota Serang
  Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str(), "iot.serangkota.go.id", 8080);

  // Inisialisasi HardwareSerial untuk PZEM-004T
  hwSerial.begin(9600, SERIAL_8N1, 16, 17); // Pin RX, TX pada ESP32

  // Inisialisasi DHT11
  dht.begin();

  // Informasi kalibrasi di Serial Monitor
  Serial.println("\n==================================================");
  Serial.println("        KALIBRASI DHT11 vs HTC-1 DIAKTIFKAN");
  Serial.println("==================================================");
  Serial.println("ANALISIS DATA (34 titik pengukuran):");
  Serial.println("");
  Serial.println("SELISIH RATA-RATA:");
  Serial.println("- Suhu: DHT11 28.9°C vs HTC-1 24.8°C -> 4.1°C");
  Serial.println("- Kelembaban: DHT11 52.4% vs HTC-1 66.2% -> 13.8%");
  Serial.println("");
  Serial.println("HASIL REGRESI LINEAR:");
  Serial.println("- Suhu: HTC-1 = 0.845 × DHT11 + 0.642 (R² = 0.897)");
  Serial.println("- Kelembaban: HTC-1 = 1.135 × DHT11 + 6.732 (R² = 0.823)");
  Serial.println("");
  Serial.println("MEAN ABSOLUTE ERROR (MAE):");
  Serial.println("- Suhu: 0.42°C");
  Serial.println("- Kelembaban: 2.87%");
  Serial.println("");
  Serial.println("KOREKSI YANG DITERAPKAN:");
  Serial.println("- Suhu Terkoreksi = DHT11 - 4.1°C");
  Serial.println("- Kelembaban Terkoreksi = DHT11 + 13.8%");
  Serial.println("==================================================\n");
}

void loop() {
  Blynk.run();
  static unsigned long previousMillisEnergy = 0;
  const long intervalEnergy = 2500; // Interval 2.5 detik untuk PZEM-004T dan DHT11

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisEnergy >= intervalEnergy) {
    previousMillisEnergy = currentMillis;
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
  int introTextLength = introText.length();
  int authorTextLength = authorText.length();

  int introStartPos = (16 - introTextLength) / 2;
  int authorStartPos = (16 - authorTextLength) / 2;

  // Tampilkan teks intro
  lcd.setCursor(introStartPos, 0);
  lcd.print(introText);
  lcd.setCursor(authorStartPos, 1);
  lcd.print(authorText);

  // Jika teks terlalu panjang, scroll teks dari kiri ke kanan
  if (introTextLength > 16 || authorTextLength > 16) {
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
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energyWh = pzem.energy(); // Wh
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  // Handle nilai NaN
  voltage = zeroIfNan(voltage);
  current = zeroIfNan(current);
  power = zeroIfNan(power);
  energyWh = zeroIfNan(energyWh);
  frequency = zeroIfNan(frequency);
  pf = zeroIfNan(pf);

  // Baca data dari DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Handle nilai NaN untuk DHT11
  humidity = zeroIfNan(humidity);
  temperature = zeroIfNan(temperature);

  // KALIBRASI DATA DHT11 - menggunakan koreksi yang benar
  float calibratedTemp = calibrateTemperature(temperature);
  float calibratedHum = calibrateHumidity(humidity);

  // Juga hitung dengan metode linear untuk perbandingan
  float linearTemp = calibrateTemperatureLinear(temperature);
  float linearHum = calibrateHumidityLinear(humidity);

  // Hitung apparent power (VA) - Daya semu
  float apparentPower = (pf == 0) ? 0 : power / pf;

  // Hitung reactive power (VAR) - Daya reaktif
  float reactivePower = (pf == 0) ? 0 : power / pf * sqrt(1 - sq(pf));

  // Tampilkan pada LCD - TAMPILAN ASLI dengan nilai terkoreksi
  lcd.clear();
  if (displayMode == 0) {
    // Tampilkan voltage dan current
    lcd.setCursor(0, 0);
    lcd.print("Volt: " + String(voltage) + "V");
    lcd.setCursor(0, 1);
    lcd.print("Curr: " + String(current) + "A");
  } else if (displayMode == 1) {
    // Tampilkan power
    lcd.setCursor(0, 0);
    lcd.print("Power: " + String(power) + "W");
    lcd.setCursor(0, 1);
    lcd.print("Freq: " + String(frequency) + "Hz");
  } else if (displayMode == 2) {
    // Tampilkan energy dalam Wh
    lcd.setCursor(0, 0);
    lcd.print("Energy: " + String(energyWh) + "Wh");
    lcd.setCursor(0, 1);
    lcd.print("PF: " + String(pf) + "  ");
  } else if (displayMode == 3) {
    // Tampilkan temperature dan humidity TERKOREKSI
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(calibratedTemp) + "C");
    lcd.setCursor(0, 1);
    lcd.print("Hum: " + String(calibratedHum) + "%");
  }

  // Ubah mode display
  displayMode = (displayMode + 1) % 4;

  // Kirim data ke Blynk - HANYA NILAI TERKOREKSI untuk suhu & kelembaban
  Blynk.virtualWrite(V0, voltage);
  Blynk.virtualWrite(V1, current);
  Blynk.virtualWrite(V2, power);
  Blynk.virtualWrite(V3, pf); // Cos Phi (Power Factor)
  Blynk.virtualWrite(V4, apparentPower); // Apparent Power (VA)
  Blynk.virtualWrite(V5, energyWh); // Total Energy (Wh)
  Blynk.virtualWrite(V6, frequency); // Frequency (Hz)
  Blynk.virtualWrite(V7, reactivePower); // Reactive Power (VAR)

  // Kirim data DHT11 TERKOREKSI ke Blynk - menggantikan nilai sebelumnya
  Blynk.virtualWrite(V8, calibratedTemp); // Temperature terkoreksi
  Blynk.virtualWrite(V9, calibratedHum);  // Humidity terkoreksi

  // Log ke Serial Monitor setiap 10 readings
  static int logCounter = 0;
  if (logCounter++ >= 10) {
    logCounter = 0;
    
    Serial.println("=== DATA KALIBRASI DHT11 ===");
    Serial.print("DHT11 Mentah     -> Temp: "); 
    Serial.print(temperature); 
    Serial.print("C, Hum: "); 
    Serial.print(humidity); 
    Serial.println("%");
    
    Serial.print("Koreksi Sederhana-> Temp: "); 
    Serial.print(calibratedTemp);
    Serial.print("C, Hum: "); 
    Serial.print(calibratedHum); 
    Serial.println("%");
    
    Serial.print("Regresi Linear   -> Temp: "); 
    Serial.print(linearTemp);
    Serial.print("C, Hum: "); 
    Serial.print(linearHum); 
    Serial.println("%");
    
    Serial.print("Selisih Koreksi  -> Temp: "); 
    Serial.print(calibratedTemp - temperature);
    Serial.print("C, Hum: "); 
    Serial.print(calibratedHum - humidity); 
    Serial.println("%");
    
    // Hitung error aktual berdasarkan metode sederhana
    float tempError = abs(calibratedTemp - linearTemp);
    float humError = abs(calibratedHum - linearHum);
    
    Serial.print("Error vs Linear  -> Temp: "); 
    Serial.print(tempError, 2);
    Serial.print("C, Hum: "); 
    Serial.print(humError, 2); 
    Serial.println("%");
    Serial.println("=============================");
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

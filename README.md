<div align="center">
   
# Smart Office Guardian - Energy & Environment Monitoring IoT System

</div>

## Overview
An advanced IoT monitoring system that tracks electrical energy consumption and environmental conditions using ESP32 microcontroller with intelligent fuzzy logic classification. The system integrates PZEM-004T for power monitoring and DHT11 for temperature/humidity sensing, with enhanced calibration algorithms and real-time fuzzy analysis for smart office optimization.

<div align="center">

| Blynk Dashboard | LCD Display |
|:------------:|:--------------:|
| <img width="240" height="533" src="https://github.com/user-attachments/assets/912dc2a3-61f9-486d-ab43-2b00a42f24fa"> | <img width="240" height="533" src="https://github.com/user-attachments/assets/38b05b3e-0596-4843-8f7a-c68e2ff991aa"> |
| **Figure 1**: Blynk Dashboard | **Figure 2**: LCD Display Interface |

</div>

## Features
- **Real-time Power Monitoring**: Voltage, current, power, energy, frequency, power factor, and reactive power
- **Environmental Sensing**: Temperature and humidity monitoring with HTC-1 reference calibration (95.8%-97.7% accuracy)
- **Fuzzy Logic Intelligence**: Dual fuzzy systems for thermal comfort and energy consumption classification
- **IoT Connectivity**: Blynk platform integration with Google Sheets data logging
- **LCD Display**: 5-mode rotating display with full text labels and fuzzy status
- **WiFi Manager**: Easy network configuration with "EcoOffice" access point
- **Data Logging**: Comprehensive serial monitoring with real-time accuracy analysis

## Hardware Components
<div align="center">

| |
|:-------------------------:|
| <img width="480" height="720" alt="Wiring Schematic" src="https://github.com/user-attachments/assets/2fa6e24f-227e-4ad9-a243-f8f93257cdf7"> |
| **Figure 3**: Hardware system monitoring connection schematic diagram |

</div>

- **ESP32 Development Board**
- **PZEM-004T Power Monitoring Module**
- **DHT11 Temperature/Humidity Sensor**
- **HTC-1 Reference Thermometer/Hygrometer**
- **16x2 I2C LCD Display** (Address: 0x27)
- **Breadboard and Jumper Wires**

## Enhanced Sensor Calibration Methodology

### Advanced Linear Regression Analysis

The system employs **enhanced linear regression analysis** based on 34 data points for superior accuracy:

#### Temperature Calibration (95.8% Accuracy):
```
HTC-1 = 0.923 × DHT11 - 1.618 (R² = 0.958)
```
- **R² = 0.958** indicates excellent linear correlation
- **Slope (0.923)**: Precise rate of change adjustment
- **Intercept (-1.618)**: Systematic offset correction

#### Humidity Calibration (97.7% Accuracy):
```
HTC-1 = 0.926 × DHT11 + 18.052 (R² = 0.977)
```
- **R² = 0.977** indicates outstanding linear relationship
- **Slope (0.926)**: Fine-tuned humidity scaling
- **Intercept (18.052)**: Significant baseline adjustment

### Real-Time Error Monitoring

**Advanced MAE Calculation** with circular buffer:
```cpp
// Circular buffer for error tracking
float tempErrors[10]; // 10 most recent temperature errors
float humErrors[10];  // 10 most recent humidity errors

// Real-time accuracy calculation
float calculateAccuracy(float mae, float range) {
  return max(0.0, 100.0 - (mae / range * 100.0));
}
```

#### Calibration Performance:
- **Temperature MAE**: 3.84°C → 95.8% accuracy
- **Humidity MAE**: 14.18% → 97.7% accuracy
- **Real-time monitoring**: Continuous accuracy assessment

## Intelligent Fuzzy Logic System

### Dual Fuzzy Classification Engine

#### 1. Thermal Comfort Classification (V10)
**Based on ASHRAE 55 & ISO 7730 Standards:**
- **COLD**: Below comfort range (16-22°C)
- **COOL**: Slightly below optimal
- **COMFORTABLE**: Optimal office range (20-26°C)
- **WARM**: Slightly above optimal  
- **HOT**: Above comfort range (28-34°C)

**8 Fuzzy Rules:**
- IF Temperature is COLD THEN Comfort is COLD
- IF Temperature is COMFORTABLE AND Humidity is COMFORTABLE THEN Comfort is COMFORTABLE
- IF Temperature is COMFORTABLE AND Humidity is DRY THEN Comfort is COOL
- IF Temperature is COMFORTABLE AND Humidity is HUMID THEN Comfort is WARM
- IF Temperature is WARM THEN Comfort is WARM
- IF Temperature is HOT THEN Comfort is HOT
- IF Temperature is COLD AND Humidity is HUMID THEN Comfort is COOL
- IF Temperature is HOT AND Humidity is HUMID THEN Comfort is HOT

#### 2. Energy Consumption Classification (V11)
**Based on IEEE 1159 Power Quality Standards:**

**13 Fuzzy Rules for Energy Efficiency:**
- **ECONOMICAL**: Low consumption with good power quality
- **NORMAL**: Standard operation within expected ranges  
- **WASTEFUL**: High consumption or poor power quality

**Input Parameters:**
- Voltage (PLN Standard: 220V ±10%)
- Power (Office consumption: 200-1500W)
- Power Factor (Industrial standard: >0.8 good)
- Reactive Power (Quality indicator)

## Enhanced Data Collection & Display

### 5-Mode LCD Display System

```cpp
// Display Modes with Full Text Labels
switch (displayMode) {
  case 0: // Voltage & Current
    lcd.print("Voltage: " + String(voltage, 1) + "V");
    lcd.print("Current: " + String(current, 3) + "A");
    break;
  case 1: // Power & Frequency
    lcd.print("Power: " + String(power, 1) + "W");
    lcd.print("Frequency: " + String(frequency, 1) + "Hz");
    break;
  case 2: // Energy & Power Factor
    lcd.print("Energy: " + String(energyWh, 1) + "Wh");
    lcd.print("Power Factor: " + String(pf, 2));
    break;
  case 3: // Temperature & Humidity
    lcd.print("Temperature: " + String(calibratedTemp, 1) + "C");
    lcd.print("Humidity: " + String(calibratedHum, 1) + "%");
    break;
  case 4: // Fuzzy Logic Status
    lcd.print("Comfort: " + tempComfort);
    lcd.print("Energy: " + energyStatus);
    break;
}
```

### Blynk Virtual Pin Mapping

| Virtual Pin | Data Type | Description |
|-------------|-----------|-------------|
| V0 | Float | Voltage (V) |
| V1 | Float | Current (A) |
| V2 | Float | Power (W) |
| V3 | Float | Power Factor |
| V4 | Float | Apparent Power (VA) |
| V5 | Float | Energy (Wh) |
| V6 | Float | Frequency (Hz) |
| V7 | Float | Reactive Power (VAR) |
| V8 | Float | Calibrated Temperature (°C) |
| V9 | Float | Calibrated Humidity (%) |
| V10 | String | Thermal Comfort Status |
| V11 | String | Energy Consumption Status |

## Installation and Setup

### Prerequisites
- **Arduino IDE** with ESP32 support
- **Required Libraries**:
  - `Blynk` - IoT platform integration
  - `LiquidCrystal_I2C` - LCD display control
  - `WiFiManager` - Network configuration
  - `PZEM004Tv30` - Power monitoring
  - `DHT sensor library` - Environmental sensing

### Hardware Configuration

```cpp
// Pin Definitions
PZEM-004T: RX=16, TX=17 (HardwareSerial UART1)
DHT11: Pin 27
LCD: I2C Address 0x27
Boot Button: Pin 0 (WiFi Reset)

// WiFi Configuration
AP_SSID: "EcoOffice"
AP_PASS: "guard14n0ff1ce"
```

### Blynk Setup
1. Use template: `TMPL6eUbLFTuj` - "Energy Monitor"
2. Configure local server: `iot.serangkota.go.id:8080`
3. Set up virtual pins V0-V11 for data visualization

## System Operation

### Enhanced Startup Sequence
1. **LCD Initialization** - EcoOffice branding display
2. **WiFi Connection** - With blinking LED feedback and IP display
3. **Sensor Calibration** - Advanced linear regression activation
4. **Blynk Connection** - Local server integration
5. **Fuzzy System Initialization** - Rule base setup
6. **Continuous Monitoring** - 3-second intervals with 5-mode display

### Real-time Monitoring Features
- **3-second sensor reading intervals**
- **5-mode LCD rotation** (3 seconds per mode)
- **Blynk real-time updates** for all parameters
- **Serial logging** every 18 seconds (6 readings)
- **Fuzzy classification** updates on V10/V11

## Technical Specifications

### Enhanced Sensor Accuracy

| Parameter | Raw DHT11 Error | After Calibration | Accuracy | Improvement |
|-----------|-----------------|-------------------|----------|-------------|
| Temperature | ±4.1°C | ±0.42°C | 95.8% | 90% |
| Humidity | ±13.8% | ±2.87% | 97.7% | 79% |

### System Performance Metrics

| Aspect | Specification |
|--------|---------------|
| Monitoring Interval | 3 seconds |
| LCD Update | 5 modes × 3 seconds |
| Blynk Update | Real-time |
| Serial Logging | Every 18 seconds |
| WiFi Timeout | 60 seconds |
| Fuzzy Update | Every reading |

### Calibration Effectiveness
- **Temperature R²**: 0.958 (Excellent correlation)
- **Humidity R²**: 0.977 (Outstanding correlation)  
- **Overall Accuracy**: 95.8%-97.7% for both parameters
- **Implementation**: Linear regression for maximum precision

## Google Sheets Integration

### Enhanced App Script Features
- **Automatic data logging** from Blynk virtual pins
- **Fuzzy classification capture** from V10 and V11
- **Advanced calculations**:
  - Current per kW analysis
  - Power Quality Score (0-100)
  - Energy cost calculation (Rp/kWh)
  - Voltage stability percentage

### Data Access
🔗 **[Live Monitoring Dashboard](https://ipb.link/energy-temperature-monitoring-data)**

<div align="center">
  
**Scan for Mobile Access:**

<img width="128" height="128" alt="QR Code" src="https://github.com/user-attachments/assets/7a710f50-6f1c-44da-a7c9-cdb8f2d4445a" />

</div>

## Applications
- **Smart Office Optimization** - Thermal comfort and energy efficiency
- **HVAC System Management** - Environmental condition tracking
- **Energy Consumption Analytics** - Fuzzy-based classification
- **IoT Research Platform** - Advanced sensor calibration studies
- **Building Management Systems** - Real-time monitoring and alerts

## Future Enhancements
- **Machine Learning Integration** - Predictive maintenance
- **Multi-zone Monitoring** - Expanded sensor networks
- **Mobile App Development** - Enhanced user interface
- **Cloud Analytics** - Advanced data processing
- **Automated Reporting** - Scheduled performance insights

## License
This project is licensed under the [UNLICENSE](https://github.com/dankehidayat/energy-temperature-monitor/blob/master/UNLICENSE).

## Author
**Danke Hidayat** - IoT & Embedded Systems Developer  
*Specializing in smart office solutions and sensor fusion technologies*

---
**Last updated**: December 2024  

**Calibration data**: Based on 34-point comprehensive analysis  

**System optimized for**: PT. Global Kreatif Inovasi smart office environment  

*For real-time sensor data and performance metrics, [access the live dashboard](https://ipb.link/energy-temperature-monitoring-data).*

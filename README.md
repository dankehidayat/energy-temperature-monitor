<div align="center">
   
# Office Energy and Environment Monitoring IoT System

</div>

## Overview
A comprehensive IoT monitoring system that tracks electrical energy consumption and environmental conditions using ESP32 microcontroller. The system integrates PZEM-004T for power monitoring and DHT11 for temperature/humidity sensing, with advanced calibration algorithms for improved accuracy.

<div align="center">

| Blynk Dashboard | Blynk Dashboard |
|:------------:|:--------------:|
| <img width="240" height="533" src="https://github.com/user-attachments/assets/912dc2a3-61f9-486d-ab43-2b00a42f24fa"> | <img width="240" height="533" src="https://github.com/user-attachments/assets/38b05b3e-0596-4843-8f7a-c68e2ff991aa"> |
| **Figure 1**: Basic overview | **Figure 2**: Detailed overview |

</div>

## Features
- **Real-time Power Monitoring**: Voltage, current, power, energy, frequency, and power factor
- **Environmental Sensing**: Temperature and humidity monitoring with HTC-1 reference calibration
- **IoT Connectivity**: Blynk platform integration for remote monitoring
- **LCD Display**: Real-time data visualization with rotating display modes
- **WiFi Manager**: Easy network configuration without hardcoding credentials
- **Data Logging**: Comprehensive serial monitoring with calibration analysis

## Hardware Components
<div align="center">

| |
|:-------------------------:|
| <img width="480" height="720" alt="Wiring Schematic" src="https://github.com/user-attachments/assets/2fa6e24f-227e-4ad9-a243-f8f93257cdf7"> |
| **Figure 3**: Hardware system monitoring connection schematic diagram |

</div>

- ESP32 Development Board
- PZEM-004T Power Monitoring Module
- DHT11 Temperature/Humidity Sensor
- HTC-1 Reference Thermometer/Hygrometer
- 16x2 I2C LCD Display
- Breadboard and Jumper Wires

## Sensor Calibration Methodology

### Linear Regression for Sensor Accuracy

The system employs **linear regression analysis** to establish the mathematical relationship between DHT11 readings and HTC-1 reference values:

#### Temperature Calibration:
```
HTC-1 = 0.845 × DHT11 + 0.642 (R² = 0.897)
```
- **R² = 0.897** indicates strong linear correlation
- **Slope (0.845)**: Rate of change between DHT11 and HTC-1 readings
- **Intercept (0.642)**: Systematic offset between sensors

#### Humidity Calibration:
```
HTC-1 = 1.135 × DHT11 + 6.732 (R² = 0.823)
```
- **R² = 0.823** indicates good linear relationship
- **Slope > 1**: DHT11 underestimates humidity changes
- **Positive intercept**: DHT11 consistently reads lower than HTC-1

### Mean Absolute Error (MAE) Analysis

**MAE Calculation**: `MAE = Σ|actual - predicted| / n`

#### Calibration Results:
- **Temperature MAE**: 0.42°C
- **Humidity MAE**: 2.87%

#### MAE Interpretation:
- **Temperature**: Average error of 0.42°C between calibrated DHT11 and HTC-1
- **Humidity**: Average error of 2.87% between calibrated readings
- **Lower MAE** indicates better prediction accuracy
- **Robust metric** less sensitive to outliers compared to RMSE

### Applied Correction Strategy

Based on analysis of 34 data points, the system implements **simple offset correction**:

```cpp
// Temperature correction
float calibrateTemperature(float rawTemp) {
  return rawTemp - 4.1; // DHT11 reads 4.1°C higher
}

// Humidity correction  
float calibrateHumidity(float rawHum) {
  return rawHum + 13.8; // DHT11 reads 13.8% lower
}
```

**Why Simple Correction?**
- More stable for real-time applications
- Minimal accuracy difference vs linear regression (MAE < 0.5°C)
- Simpler implementation with better performance

## Data Collection and Analysis

### Calibration Dataset
- **34 simultaneous measurement points**
- DHT11 vs HTC-1 comparison across varying conditions
- Statistical analysis for regression coefficients

### Live Data Monitoring
Access real-time and historical data:

🔗 **[Energy & Temperature Monitoring Data](https://ipb.link/energy-temperature-monitoring-data)**

Access by QR:

<img width="128" height="128" alt="image" src="https://github.com/user-attachments/assets/7a710f50-6f1c-44da-a7c9-cdb8f2d4445a" />

## Installation and Setup

### Prerequisites
- Arduino IDE with ESP32 support
- Required libraries:
  - Blynk
  - LiquidCrystal_I2C
  - WiFiManager
  - PZEM004Tv30
  - DHT sensor library

### Configuration
1. Update Blynk template ID and auth token
2. Configure WiFi credentials in setup
3. Connect hardware according to pin definitions
4. Upload code to ESP32

### Pin Connections
```cpp
PZEM-004T: RX=16, TX=17
DHT11: Pin 27
LCD: I2C Address 0x27
Boot Button: Pin 0
```

## System Operation

### Startup Sequence
1. LCD initialization and intro display
2. WiFi connection with visual feedback
3. Sensor calibration activation
4. Blynk server connection
5. Continuous monitoring cycle

### Display Modes
The system cycles through 4 display modes:
1. Voltage and Current
2. Power and Frequency  
3. Energy and Power Factor
4. Temperature and Humidity (Calibrated)

### Data Transmission
- **Blynk Virtual Pins V0-V9** for all parameters
- **Serial Monitor** logging every 10 readings
- **Real-time calibration error monitoring**

## Technical Specifications

### Sensor Accuracy After Calibration
| Parameter | Raw DHT11 Error | After Calibration | Improvement |
|-----------|-----------------|-------------------|-------------|
| Temperature | ±4.1°C | ±0.42°C | 90% |
| Humidity | ±13.8% | ±2.87% | 79% |

### Monitoring Intervals
- **Sensor Reading**: 2.5 seconds
- **LCD Update**: Rotating every 2.5 seconds  
- **Blynk Update**: Real-time
- **Serial Log**: Every 25 seconds (10 readings)

## Performance Metrics

### Calibration Effectiveness
- **Temperature R²**: 0.897 (Strong correlation)
- **Humidity R²**: 0.823 (Good correlation)  
- **Overall MAE**: <3% for both parameters
- **Implementation**: Simple offset for stability

### System Reliability
- **WiFi Auto-reconnect** with manager
- **NaN value handling** for sensor faults
- **Circular buffer** for error tracking
- **Visual feedback** during operation

## Applications
- Office energy consumption monitoring
- HVAC system performance tracking
- Environmental condition logging
- IoT research and education
- Energy efficiency analysis

## Future Enhancements
- Additional sensor integration
- Cloud data storage
- Mobile app development
- Predictive maintenance features
- Energy consumption analytics

## License
This code is built under [UNLICENSE](https://github.com/dankehidayat/energy-temperature-monitor/blob/master/UNLICENSE).

## Author
**Danke Hidayat** - IoT System Developer

---
*Last updated: 4th October 2025*  
*Calibration data based on 34-point analysis. For more sensor data information, [click here](https://ipb.link/energy-temperature-monitoring-data).*

*System is optimized for PT. Global Kreatif Inovasi office environment.*

# Energy and Temperature Monitoring IoT Project

![IoT Project](https://img.shields.io/badge/Project-Energy%20Monitoring-blue)
![ESP32](https://img.shields.io/badge/Microcontroller-ESP32-green)
![Blynk](https://img.shields.io/badge/Platform-Blynk-yellow)

## 📋 Project Overview

A comprehensive energy monitoring system that tracks electrical parameters and environmental conditions using ESP32 microcontroller. The system provides real-time data visualization through both an LCD display and the Blynk mobile application.

## 🛠 Hardware Components

| Component | Purpose | Connection |
|-----------|---------|------------|
| **ESP32** | Main microcontroller | - |
| **PZEM-004T** | Energy monitoring sensor | RX: GPIO16, TX: GPIO17 |
| **DHT11** | Temperature & Humidity sensor | GPIO27 |
| **LCD I2C** | 16x2 Display | I2C Address: 0x27 |
| **Boot Button** | WiFi configuration reset | GPIO0 |

## 📊 Monitored Parameters

### Electrical Parameters (PZEM-004T)
- **Voltage** (V) - AC voltage reading
- **Current** (A) - Current consumption
- **Power** (W) - Active power
- **Energy** (Wh) - Total energy consumption
- **Frequency** (Hz) - AC frequency
- **Power Factor** - Efficiency of power usage
- **Apparent Power** (VA) - Total power in circuit
- **Reactive Power** (VAR) - Non-working power

### Environmental Data (DHT11)
- **Temperature** (°C) - Ambient temperature
- **Humidity** (%) - Relative humidity

## 🔧 Software Features

### WiFi Management
- **WiFiManager** for easy configuration
- Access Point: "Energy IoT" with password "energyiot"
- 60-second configuration timeout
- Hardware reset via GPIO0 button

### Data Display
- **LCD Display**: Rotating display of all parameters
- **Blynk App**: Real-time remote monitoring
- **Super Chart**: Historical data visualization

### Data Handling
- NaN value protection
- 2.5-second update interval
- Multiple display modes

## 🖥 Blynk Configuration

### Virtual Pins Mapping
| Virtual Pin | Parameter | Unit |
|-------------|-----------|------|
| V0 | Voltage | V |
| V1 | Current | A |
| V2 | Power | W |
| V3 | Power Factor | - |
| V4 | Apparent Power | VA |
| V5 | Energy | Wh |
| V6 | Frequency | Hz |
| V7 | Reactive Power | VAR |
| V8 | Temperature | °C |
| V9 | Humidity | % |

### Blynk Template
- **Template ID**: `TMPL6eUbLFTuj`
- **Template Name**: "Energy Monitor"
- **Server**: `iot.serangkota.go.id:8080`

## 📝 Code Structure

### Main Functions

#### `setup()`
- Initializes all components
- Starts WiFi configuration portal
- Connects to Blynk server
- Shows introductory display

#### `loop()`
- Runs Blynk operations
- Updates sensor readings every 2.5 seconds
- Manages display rotation

#### `showEnergyInfo()`
- Reads all sensor data
- Handles NaN values
- Displays data on LCD in rotating modes
- Sends data to Blynk

#### `checkBoot()`
- Monitors boot button for WiFi reset
- Long press (5+ seconds) erases WiFi settings

## 🔄 Display Modes

The LCD cycles through 4 display modes:

1. **Mode 0**: Voltage and Current
2. **Mode 1**: Power and Frequency  
3. **Mode 2**: Energy and Power Factor
4. **Mode 3**: Temperature and Humidity

## 🚀 Setup Instructions

### Hardware Setup
1. Connect PZEM-004T to ESP32:
   - RX → GPIO16
   - TX → GPIO17
2. Connect DHT11 to GPIO27
3. Connect LCD I2C to I2C pins
4. Connect boot button to GPIO0 and GND

### Software Setup
1. Upload the code to ESP32
2. Connect to "Energy IoT" WiFi network
3. Configure your WiFi credentials
4. Open Blynk app to monitor data

## 📈 Features

- **Real-time Monitoring**: Live data from all sensors
- **Historical Data**: Blynk Super Chart for trend analysis
- **Easy Configuration**: Web-based WiFi setup
- **Robust Operation**: Automatic error handling and restart
- **Local Display**: Standalone operation without internet
- **Remote Access**: Worldwide monitoring via Blynk

## 🔧 Troubleshooting

- **WiFi Connection Issues**: Press and hold boot button for 5+ seconds to reset
- **Sensor Reading Errors**: Check wiring and sensor connections
- **LCD Not Displaying**: Verify I2C address and connections
- **Blynk Connection**: Ensure correct authentication token and server

## 📊 Applications

- Office energy consumption monitoring
- Home energy management
- Industrial equipment monitoring
- Research and data logging
- Smart building applications

---

**Developer**: Danke Hidayat  
**Project**: Office Monitoring IoT  
**Version**: 2.0  
**Last Updated**: 2025

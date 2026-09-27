# ❤️ ESP32 Health Monitoring System

An embedded health monitoring system built using **ESP32, MAX30100, DHT11, 16×2 I2C LCD, Bluetooth, and relay control**.

![ESP32](https://img.shields.io/badge/Platform-ESP32-blue)
![Arduino](https://img.shields.io/badge/Framework-Arduino-00979D)
![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-orange)
![Bluetooth](https://img.shields.io/badge/Connectivity-Bluetooth-blueviolet)
![Status](https://img.shields.io/badge/Status-Prototype-green)

---

## 📌 Project Overview

The **ESP32 Health Monitoring System** is an embedded electronics project designed to monitor basic health parameters in real time.

The system uses a **MAX30100 pulse oximeter sensor** to measure heart rate and estimate SpO₂, while a **DHT11 sensor** measures temperature.

The measured values are displayed on a **16×2 I2C LCD** and transmitted wirelessly using the ESP32's built-in Bluetooth.

An automatic relay control system is also implemented. The relay is activated when the measured heart rate or temperature reaches the configured threshold.

---

## ✨ Features

- ❤️ Heart rate monitoring
- 🩸 Approximate SpO₂ measurement
- 🌡️ Temperature monitoring
- 📟 16×2 I2C LCD display
- 📱 Bluetooth wireless monitoring
- 🔌 Automatic relay control
- ⚠️ High heart-rate detection
- 🌡️ High-temperature detection
- 🔄 Continuous sensor monitoring
- 💻 ESP32-based embedded system

---

## 🧰 Hardware Requirements

| Component | Quantity |
|---|:---:|
| ESP32 DevKit V1 | 1 |
| MAX30100 Pulse Oximeter | 1 |
| DHT11 Temperature Sensor | 1 |
| 16×2 I2C LCD | 1 |
| 5V Relay Module | 1 |
| Breadboard | 1 |
| Jumper Wires | As required |

---

## 🔌 Pin Configuration

### MAX30100

| MAX30100 | ESP32 |
|---|---|
| VIN | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### DHT11

| DHT11 | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| DATA | GPIO 27 |

### 16×2 I2C LCD

| LCD | ESP32 |
|---|---|
| VCC | 5V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Relay Module

| Relay | ESP32 |
|---|---|
| IN | GPIO 26 |
| VCC | 5V |
| GND | GND |

---

📊 Parameters Monitored
Parameter
Sensor
Output
Heart Rate
MAX30100
BPM
SpO₂
MAX30100
%
Temperature
DHT11
°C

🚨 Relay Control
The relay is activated when either of the following conditions is reached:
Heart Rate ≥ 95 BPM
Temperature ≥ 37°C
Control Logic
IF BPM >= 95
       OR
   Temperature >= 37°C

THEN
   Relay = ON

ELSE
   Relay = OFF
   
Relay Conditions
Condition
Relay
BPM < 95 and Temperature < 37°C
OFF
BPM ≥ 95
ON
Temperature ≥ 37°C
ON
BPM ≥ 95 and Temperature ≥ 37°C
ON

📟 LCD Display
The LCD alternates between the heart-rate screen and temperature screen.
Heart Rate Screen
┌────────────────┐
│ BPM: 75        │
│ SpO2: 98%      │
└────────────────┘
Temperature Screen
┌────────────────┐
│ Temperature    │
│ 36.5°C         │
└────────────────┘
When Finger Is Not Detected
┌────────────────┐
│ Place Finger   │
│ on MAX30100    │
└────────────────┘
📱 Bluetooth Monitoring
The ESP32 Bluetooth device name is:
HEALTH_MONITOR
Example Bluetooth output:
====================
HEALTH MONITOR
====================
BPM: 75.0
SpO2: 98.0%
Temperature: 36.5 C
--------------------
VALUES NORMAL
RELAY: OFF
====================
When high BPM is detected:
HIGH BPM
BPM >= 95
RELAY: ON
When high temperature is detected:
HIGH TEMPERATURE
TEMP >= 37 C
RELAY: ON


💻 Software Requirements
Development Environment
Arduino IDE
ESP32 Board Package
Libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHTesp.h>
#include <BluetoothSerial.h>
#include <math.h>
The MAX30100 is accessed directly through I2C registers, so a dedicated MAX30100 library is not required for this implementation.


⚙️ Working Principle
The ESP32 initializes the MAX30100, DHT11, LCD, relay, and Bluetooth.
The MAX30100 collects infrared and red-light sensor data.
The ESP32 processes the sensor readings to estimate BPM and SpO₂.
The DHT11 measures temperature.
The measured values are displayed on the LCD.
The same information is transmitted through Bluetooth.
The ESP32 checks the configured threshold conditions.
The relay turns ON when BPM reaches 95 BPM or higher.
The relay also turns ON when temperature reaches 37°C or higher.
The relay remains OFF when both parameters are below their thresholds.


🚀 Getting Started
1. Install Arduino IDE
Install the Arduino IDE and add support for ESP32 boards.
2. Connect the Hardware
Connect the components according to the pin configuration provided above.
3. Open the Code
Open:
Heart_Rate_Sensor.ino
in Arduino IDE.
4. Select the ESP32 Board
In Arduino IDE, select your ESP32 board from:
Tools → Board → ESP32
5. Select the COM Port
Connect the ESP32 to your computer and select the correct COM port.
6. Upload the Program
Upload the program to the ESP32.
7. Open Serial Monitor
Set the Serial Monitor baud rate to:
115200
8. Connect Through Bluetooth
Search for the Bluetooth device:
HEALTH_MONITOR
Then place your finger on the MAX30100 sensor.


📁 Project Structure
heart_rate_sensor/
│
├── Heart_Rate_Sensor.ino
└── README.md


🔮 Future Improvements
📱 Dedicated mobile application
☁️ IoT/cloud monitoring
📊 Real-time graphical data
💾 Health data storage
🔔 Mobile notifications
📈 Improved heart-rate filtering
🩸 Improved SpO₂ algorithm
🌐 Blynk dashboard integration
📋 Patient data logging


⚠️ Disclaimer
This project is developed for educational, experimental, and prototype purposes only.
The heart-rate and SpO₂ calculations used in this project are simplified and should not be considered medically accurate.
This system should not be used for medical diagnosis, treatment, or clinical decision-making.
For medical applications, certified medical-grade sensors and clinically validated algorithms are required.


👨‍💻 Project Information
Project Name: ESP32 Health Monitoring System
Repository: Heart Rate Sensor
Platform: ESP32
Framework: Arduino
Programming Language: C/C++
Communication: I2C + Bluetooth
Project Type: Embedded Systems / IoT / Health Monitoring


⭐ Support
If you find this project useful, consider giving the repository a ⭐ star on GitHub.

## 🏗️ System Architecture

```text
                  ┌─────────────────┐
                  │    MAX30100     │
                  │   BPM + SpO₂    │
                  └────────┬────────┘
                           │
                           │ I2C
                           ▼
┌──────────────┐     ┌───────────────┐
│    DHT11     │────►│               │
│ Temperature  │     │     ESP32     │
└──────────────┘     │               │
                     └───┬─────┬─────┘
                         │     │
              ┌──────────┘     └──────────┐
              ▼                           ▼
       ┌─────────────┐             ┌─────────────┐
       │  I2C LCD    │             │  Bluetooth  │
       │   Display   │             │  Monitoring │
       └─────────────┘             └─────────────┘
                         │
                         ▼
                   ┌───────────┐
                   │   Relay   │
                   │  Control  │
                   └───────────┘                                                                                                                                                                                               ## parameter

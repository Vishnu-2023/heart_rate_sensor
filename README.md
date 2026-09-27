# heart_rate_sensor
code for checking without any library files
❤️ ESP32 Health Monitoring System

«An embedded health monitoring system using ESP32, MAX30100, DHT11, LCD, Bluetooth, and automatic relay control.»

"ESP32" (https://img.shields.io/badge/Platform-ESP32-blue)
"Arduino" (https://img.shields.io/badge/Framework-Arduino-00979D)
"Language" (https://img.shields.io/badge/Language-C%2FC%2B%2B-orange)
"Bluetooth" (https://img.shields.io/badge/Connectivity-Bluetooth-blueviolet)
"Status" (https://img.shields.io/badge/Status-Prototype-green)

---

📌 Overview

The ESP32 Health Monitoring System is an embedded electronics project designed to monitor basic health parameters in real time.

The system uses a MAX30100 pulse oximeter sensor to measure heart rate and estimate SpO₂, while a DHT11 sensor measures temperature. The measured values are displayed on a 16×2 I2C LCD and transmitted wirelessly through the ESP32's built-in Bluetooth.

An automatic relay control system is also implemented. The relay is activated when the measured heart rate or temperature reaches the configured threshold.

---

✨ Features

- ❤️ Real-time heart rate monitoring
- 🩸 Approximate SpO₂ measurement
- 🌡️ Temperature monitoring
- 📟 16×2 I2C LCD display
- 📱 Bluetooth wireless monitoring
- 🔌 Automatic relay control
- ⚠️ High heart-rate detection
- 🌡️ High-temperature detection
- 🔄 Continuous sensor monitoring
- 💻 ESP32-based embedded implementation

---

🧰 Hardware Requirements

Component| Quantity
ESP32 DevKit V1| 1
MAX30100 Pulse Oximeter| 1
DHT11 Temperature Sensor| 1
16×2 I2C LCD| 1
5V Relay Module| 1
Breadboard| 1
Jumper Wires| As required

---

🔌 Pin Configuration

MAX30100

MAX30100| ESP32
VIN| 3.3V
GND| GND
SDA| GPIO 21
SCL| GPIO 22

DHT11

DHT11| ESP32
VCC| 3.3V
GND| GND
DATA| GPIO 27

I2C LCD

LCD| ESP32
VCC| 5V
GND| GND
SDA| GPIO 21
SCL| GPIO 22

Relay Module

Relay| ESP32
IN| GPIO 26
VCC| 5V
GND| GND

---

⚙️ System Architecture

                 ┌─────────────────┐
                 │   MAX30100      │
                 │ BPM + SpO₂      │
                 └────────┬────────┘
                          │
                          │ I2C
                          ▼
┌──────────────┐    ┌───────────────┐
│    DHT11     │───►│               │
│ Temperature  │    │     ESP32     │
└──────────────┘    │               │
                    └───┬─────┬─────┘
                        │     │
             ┌──────────┘     └──────────┐
             ▼                           ▼
      ┌─────────────┐             ┌─────────────┐
      │  I2C LCD    │             │  Bluetooth  │
      │   Display   │             │  Monitoring  │
      └─────────────┘             └─────────────┘
                        │
                        ▼
                  ┌───────────┐
                  │   Relay   │
                  │  Control  │
                  └───────────┘

---

📊 Monitoring Parameters

The system monitors three primary parameters:

Parameter| Sensor| Output
Heart Rate| MAX30100| BPM
SpO₂| MAX30100| %
Temperature| DHT11| °C

---

🚨 Automatic Relay Control

The relay is controlled using the following thresholds:

Parameter| Threshold| Relay
Heart Rate| ≥ 95 BPM| ON
Temperature| ≥ 37°C| ON
Both below threshold| —| OFF

Control Logic

IF BPM >= 95
        OR
   Temperature >= 37°C
THEN
   Relay = ON
ELSE
   Relay = OFF

---

📟 LCD Interface

The LCD alternates between the heart-rate and temperature screens.

Heart Rate

┌────────────────┐
│ BPM: 75        │
│ SpO2: 98%      │
└────────────────┘

Temperature

┌────────────────┐
│ Temperature    │
│ 36.5°C         │
└────────────────┘

When no finger is detected:

┌────────────────┐
│ Place Finger   │
│ on MAX30100    │
└────────────────┘

---

📱 Bluetooth Monitoring

The ESP32 creates a Bluetooth device named:

HEALTH_MONITOR

Example output:

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

When a threshold is exceeded:

HIGH BPM
BPM >= 95
RELAY: ON

or:

HIGH TEMPERATURE
TEMP >= 37 C
RELAY: ON

---

💻 Software & Libraries

Development Environment

- Arduino IDE
- ESP32 Board Package

Libraries

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHTesp.h>
#include <BluetoothSerial.h>
#include <math.h>

The MAX30100 is accessed directly through I2C registers, so a dedicated MAX30100 library is not required for this implementation.

---

🔄 Working Process

1. The ESP32 initializes the MAX30100, DHT11, LCD, relay, and Bluetooth.
2. The MAX30100 continuously collects infrared and red-light sensor data.
3. The ESP32 processes the sensor data to estimate BPM and SpO₂.
4. The DHT11 periodically measures temperature.
5. The current values are displayed on the LCD.
6. The same data is transmitted through Bluetooth.
7. The ESP32 checks the configured threshold conditions.
8. The relay turns ON when BPM reaches 95 BPM or higher or temperature reaches 37°C or higher.

---

🚀 Getting Started

1. Clone the Repository

git clone https://github.com/your-username/heart-rate-sensor.git

2. Open the Project

Open:

Heart_Rate_Sensor.ino

using Arduino IDE.

3. Select ESP32

Select your ESP32 board from:

Tools → Board → ESP32

4. Select COM Port

Connect the ESP32 to your computer and select the appropriate COM port.

5. Upload

Upload the program to the ESP32.

6. Monitor

Open Serial Monitor at:

115200 baud

Then connect your phone to the Bluetooth device:

HEALTH_MONITOR

Place your finger on the MAX30100 and monitor the readings.

---

📁 Project Structure

heart-rate-sensor/
│
├── Heart_Rate_Sensor.ino
├── README.md
└── LICENSE

---

🔮 Future Enhancements

- 📱 Dedicated mobile application
- ☁️ IoT/cloud data monitoring
- 📊 Real-time health-data graphs
- 💾 Historical data storage
- 🔔 Mobile notifications
- 📈 Improved BPM filtering
- 🩸 Improved SpO₂ calculation
- 🔐 Secure remote monitoring
- 🌐 Blynk/IoT dashboard integration

---

⚠️ Disclaimer

This project is developed for educational, experimental, and prototype purposes only.

The heart-rate and SpO₂ values are based on a simplified signal-processing implementation and should not be considered medically accurate or used for diagnosis, treatment, or clinical decision-making.

For medical applications, certified medical-grade hardware and clinically validated algorithms are required.

---

👨‍💻 Project Information

Project: ESP32 Health Monitoring System
Repository: Heart Rate Sensor
Platform: ESP32
Framework: Arduino
Communication: I2C + Bluetooth
Project Type: Embedded Systems / IoT / Health Monitoring

---

⭐ Support

If you find this project useful, consider giving the repository a ⭐ star on GitHub.

# ESPlant

#### Project map:
* ***ESP8266 code***
- [Web application](https://github.com/bSienkiewicz/esplant-web)
---

**ESPlant** *(01 2023)* is a system designed to monitor and control soil moisture levels for plants using the ESP8266 microcontroller. 

Developed as a part of a university project for the "Software Engineering" class, ESPlant utilizes cheap resistance soil moisture sensors to measure soil moisture levels. When the moisture level falls below a certain threshold, the system activates a 12V pump to water the plant, controlled by an attached relay. Water level in the tank was measured with a simple ultrasonic transducer.

# Features
- Soil Moisture Monitoring: Measures soil moisture levels using inexpensive soil moisture sensors.
- Automatic Watering System: Activates a pump to water the plant when moisture levels fall below a predefined threshold.
- Relay Control: Controls the pump using an attached relay for efficient watering.
- Easy Integration: Utilizes the ESP8266 microcontroller for easy deployment and integration.

# Project Components
- ESP8266 Code: Firmware for the ESP8266 microcontroller to read sensor data and control the pump.
- Hardware Setup: Includes soil moisture sensors, relay module, ultrasonic transducer and 12V pump for automated watering.

![image](https://github.com/bSienkiewicz/ESPlant/assets/50502786/16ca0f82-469f-4b67-a103-c11460caff8a)


# Setup and Installation
1. Compile and upload Firmware to ESP8266:
- Flash the provided firmware code onto the ESP8266 microcontroller.
- Configure Wi-Fi credentials and other settings as required.

2. Hardware Configuration:

**Moisture sensor:**
- A0: Analog input for moisture sensor.
  
**Ultrasonic transducer:**
- D6: Trigger pin
- D5: Echo pin
  
**Relay:**
- D1: Relay out signal

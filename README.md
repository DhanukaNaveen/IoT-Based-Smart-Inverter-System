# IoT-Based Smart Inverter System

## Introduction
This project is an IoT-enabled smart inverter system designed to manage energy usage efficiently. It monitors battery levels, prioritizes loads dynamically, integrates renewable energy sources like solar panels, and provides real-time data visualization via a web interface.

## Circuit Design

The circuit design for the IoT-Based Smart Inverter System incorporates several key components to monitor and manage energy usage effectively:

- **Sensors**:
  - **PZEM-0041 (100A)**: Monitors AC voltage, AC current, frequency, and power factor.
  - **12V DC Voltage Sensor**: Measures the DC voltage from the battery.
  - **DHT11**: Monitors temperature and humidity to ensure safe operating conditions.

- **Relay Control**:
  - **5 Relays**: Used for dynamic load management, prioritizing high and low-priority loads based on energy availability.

- **Buzzer**: Provides real-time audio alerts for important system statuses and alerts.


### Circuit Diagram
The complete diagrams and explanations are available in the `circuit-diagrams/` folder.

![Circuit Design](./images/Circuit%20Design.png)

## App Interface

Below is the user interface of the Smart Inverter App, which allows users to monitor and manage the inverter system through a web interface.

![Smart Inverter App Interface](./images/Smart%20Inverter%20App%20Interface.png)

## Features
- Load prioritization and management using relays.
- Renewable energy integration (solar panels).
- Voice assistant for alerts and notifications.
- Real-time web-based monitoring.

## Technologies Used

### Hardware:
- ESP32 microcontroller
- Voltage, current, and temperature sensors
- Solar panels
- Relay modules
- 12V car battery
- DC-to-AC inverter circuit
- Voice module

### Software:
- Embedded C/C++ (ESP32 firmware)
- MQTT for IoT communication
- Web interface (HTML/JavaScript)
- Predictive analytics (optional with Python)

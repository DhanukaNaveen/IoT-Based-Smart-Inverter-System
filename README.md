# IoT-Based Smart Inverter System

## Introduction
This project is an IoT-enabled smart inverter system designed to manage energy usage efficiently. It monitors battery levels, prioritizes loads dynamically, integrates renewable energy sources like solar panels, and provides real-time data visualization via a web interface.

## Circuit Design
- **DC-to-AC Inverter**: Converts 12V DC to AC power.
- **Sensors**: Monitors voltage, current, and temperature.
- **Relay Control**: 4 high-priority and 2 low-priority relays for load management.
- **Voice Module**: Provides real-time audio alerts.

### Circuit Diagram
The complete diagrams and explanations are available in the `circuit-diagrams/` folder.

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

## Installation & Setup
1. Clone the repository:
   ```bash
   git clone https://github.com/DhanukaNaveen/IoT-Based-Smart-Inverter-System.git

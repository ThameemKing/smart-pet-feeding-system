# Smart Pet Feeding System  
### Embedded IoT & Real-Time Control using ESP32 (C++)

An embedded IoT-based automated pet feeding system integrating WiFi networking, HTTP server functionality, NTP-based scheduling, and deterministic stepper motor control.

---

## 🚀 Overview

This project implements a real-time automated pet feeding system using ESP32 firmware written in C++.  

It enables:

- Remote feeding via web interface  
- Scheduled feeding with NTP time synchronization  
- Deterministic motor actuation  
- ML-assisted portion logic using TensorFlow.js  

---

## 🏗 System Architecture

```
+----------------------+
|  Web Browser Client  |
|  (HTML + TF.js)      |
+----------+-----------+
           |
           | HTTP Request
           v
+----------------------+
|      ESP32 MCU       |
|  C++ Firmware        |
|  - HTTP Server       |
|  - NTP Sync          |
|  - Scheduler         |
+----------+-----------+
           |
           | GPIO Signals
           v
+----------------------+
|  Stepper Motor Driver|
+----------+-----------+
           |
           v
+----------------------+
|  Food Dispensing     |
|  Mechanism           |
+----------------------+
```

## ⚙️ Key Engineering Features

- Embedded firmware development in C++
- Deterministic stepper motor sequencing
- Calibrated feed-to-step mapping
- Non-blocking request handling
- NTP-based minute-level scheduling precision
- Structured scheduling system using C++ vectors
- Stateful actuator execution control

---

## 🛠 Technologies Used

- C++
- ESP32 WiFi stack
- HTTP Server
- NTP time synchronization
- TensorFlow.js (MobileNet)
- HTML/CSS/JavaScript

---

## 🔧 Setup Instructions

1. Install ESP32 board support in Arduino IDE.
2. Update WiFi credentials in firmware.
3. Connect stepper motor and driver to defined GPIO pins.
4. Upload firmware to ESP32.
5. Access device IP via browser.

---

## 📌 Future Improvements

- On-device ML inference
- Persistent storage using flash memory
- OTA firmware updates
- Motor load monitoring and safety checks

---

## 📄 License

MIT License

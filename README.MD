
# BEAM: Bilirubin Evaluation and Analysis Meter

![BEAM Header](https://via.placeholder.com/800x200?text=BEAM+-+Bilirubin+Evaluation+and+Analysis+Meter)

> **⚠️ MEDICAL DISCLAIMER:** This project is an open-source, experimental prototype designed for educational, hackathon, and demonstration purposes only. It is **NOT** an FDA-approved medical device and should absolutely **NOT** be used to diagnose, monitor, or treat neonatal jaundice or any other medical condition.

BEAM is a low-cost, IoT-enabled Transcutaneous Bilirubin (TcB) meter prototype. It non-invasively estimates bilirubin levels by analyzing the optical absorption of light reflected from the skin, using a multi-spectral sensor and a cold white LED. The device features an ESP32-driven hardware module and a Next.js web dashboard for real-time data monitoring.

## 📸 Project Gallery

### The Final Device
![Final Output](https://via.placeholder.com/600x400?text=Final+Assembled+Device)

### PCB Design
![PCB Layout](https://via.placeholder.com/600x400?text=PCB+Routing+Layout)

### 3D Model Enclosure
![3D Model](https://via.placeholder.com/600x400?text=3D+Enclosure+Render)

---

## 🌟 Features
* **Non-Invasive Optical Engine:** Utilizes the SparkFun AS7343 14-channel spectral sensor paired with a broad-spectrum Cold White LED.
* **Dual-Wavelength Math:** Calculates optical density using targeted Blue (~450nm) and Red (~650nm) absorption channels.
* **On-Board UI:** 1.8" TFT SPI screen (ST7735) displays real-time absorbance data, history tracking, and the final TcB score.
* **IoT Dashboard Integration:** Automatically syncs scans via WiFi to a Next.js frontend using REST APIs.
* **Over-The-Air (OTA) Updates:** Flash new firmware wirelessly without dismantling the enclosure.

## 🛠️ Hardware Requirements
* **Microcontroller:** ESP32 Development Board (e.g., NodeMCU-32S)
* **Sensor:** SparkFun AS7343 Multi-Spectral Sensor (I2C)
* **Display:** 1.8" SPI TFT LCD (ST7735 controller)
* **Light Source:** Cold White LED (connected via GPIO 26)
* **Input:** Tactile Push Button (connected via GPIO 27)

## 🚀 Quick Start (Hardware)
1. **Wire the components:** Follow the schematic detailed in `documentation.md`.
2. **Install Libraries:** In the Arduino IDE, install `Adafruit_ST7735`, `Adafruit_GFX`, and `SparkFun_AS7343`.
3. **Configure WiFi:** Update the SSID and password credentials in the `.ino` file.
4. **Flash the ESP32:** Upload the code via USB for the first time.
5. **Calibrate:** Refer to the "White/Yellow Paper Protocol" in the documentation to set your zero-point multipliers.

## 💻 Web Dashboard
The web dashboard is built with Next.js (App Router) and Tailwind CSS.
1. Navigate to the `/web` folder.
2. Run `npm install`.
3. Run `npm run dev` to start the local server.
4. Ensure your ESP32 is pointed to your local IPv4 address (e.g., `http://192.168.x.x:3000/api/readings`).

## 📖 Comprehensive Documentation
For a deep dive into the clinical problem, mathematical methodology, optical physics, and exact code breakdown, please see [documentation.md](documentation.md).

## 📄 License
This project is open-source under the MIT License. See the LICENSE file for details.
# BEAM: Comprehensive Project Documentation

## 1. The Premise & Problem Statement
Neonatal jaundice affects over 60% of newborns worldwide. It is caused by an accumulation of **bilirubin**, a yellow pigment produced during the normal breakdown of red blood cells. If left unmonitored and untreated, severe jaundice can lead to kernicterus—a form of irreversible brain damage.

**The Current Standards of Care:**
1. **Total Serum Bilirubin (TSB):** The gold standard, but it requires drawing blood (a heel stick). It is painful, invasive, causes parental anxiety, and requires laboratory infrastructure.
2. **Transcutaneous Bilirubinometers (TcB):** Non-invasive devices that press against the infant's forehead or sternum to measure optical reflection. While effective for screening, clinical-grade TcB meters (like the Dräger Jaundice Meter JM-105) cost thousands of dollars, making them inaccessible in many low-resource or rural healthcare settings.

## 2. The Proposal
**BEAM (Bilirubin Evaluation and Analysis Meter)** proposes a low-cost, open-source, IoT-enabled alternative. By leveraging modern, inexpensive multi-spectral I2C sensors (originally designed for agricultural and mobile display color-matching) and standard microcontroller hardware, BEAM aims to replicate the core optical physics of commercial TcB meters at a fraction of the cost. 

Furthermore, BEAM modernizes the form factor by pushing data directly to a Next.js web dashboard via WiFi, enabling remote monitoring and digital record-keeping.

---

## 3. Methodology & Hardware Architecture
The system architecture is divided into the Physical Node (ESP32) and the Cloud/Local Dashboard.

### The Hardware Stack
* **The Brain (ESP32):** Handles I2C communication with the sensor, SPI rendering for the TFT display, physical button debouncing, and HTTP WiFi POST requests.
* **The "Eye" (SparkFun AS7343):** A 14-channel spectral sensor. We isolate the F3/F4 channels (Blue light) and F7/F8 channels (Red light) to measure precise wavelengths.
* **The "Sun" (Cold White LED):** Used as the excitation source. A cold white LED is fundamentally a blue diode covered by a yellow phosphor. It emits a massive spike in the ~450nm blue range (perfect for exciting bilirubin) while also providing a broad red/green baseline.
* **The UI (1.8" ST7735 TFT):** Provides an immediate, on-device readout of Raw Data, Absorbance limits, and the calculated TcB score, operating on a Green Tab offset initialization.

---

## 4. The Mathematics & Optical Physics

### The Physics of Bilirubin
Bilirubin is a yellow-orange pigment. In optical physics, a material appears yellow because it reflects red and green light while heavily **absorbing blue light (around 450nm - 460nm)**. 
Therefore, if a baby has a high concentration of bilirubin in their subcutaneous tissue, their skin will absorb more blue light. 

### The Dual-Wavelength Method
Skin tone (melanin concentration) and dermal thickness vary wildly between patients. If we only measured blue light reflection, darker skin would artificially inflate the jaundice score. To correct this, we use a dual-wavelength optical density formula:
1. **Blue Channel (The Target):** Highly absorbed by Bilirubin.
2. **Red Channel (The Baseline):** Barely absorbed by Bilirubin, but equally affected by skin melanin.

### The Algorithm
First, we convert the raw reflected light values into **Optical Density (Absorbance)** using a modified Beer-Lambert Law approximation. Since the AS7343 is a 16-bit sensor, its maximum value is 65535.

`A = log10( 65535 / (RawLight + 1) )`

Next, we subtract the Red baseline from the Blue target, applying calibration multipliers ($M_1$ and $M_2$):

`TcB = (A_blue * M_1) - (A_red * M_2)`

*As blue light reflection drops (due to bilirubin absorption), `A_blue` spikes, driving the TcB score higher.*

---

## 5. Calibration Protocol (The Hackathon "Paper" Test)
Because BEAM is a prototype without clinical trials, the exact $M_1$ and $M_2$ multipliers cannot be mapped to true human serum levels. To prove the optical engine functions for judges, we calibrate using paper pigments to simulate healthy and jaundiced states.

### Step 1: The "Healthy" Baseline (Zeroing)
* **Material:** A stack of 5-10 plain white printer sheets (to prevent table reflection).
* **Physics:** White reflects all wavelengths. The sensor reads maximum Blue and Red light. 
* **Action:** Note the `A_blue` value. Modify the code to subtract this exact baseline so the white paper outputs a `0.0` TcB score.

### Step 2: The "Jaundiced" Target
* **Material:** A yellow Post-it note placed on top of the white stack.
* **Physics:** The yellow pigment allows red light to bounce back but "eats" the blue light. The `A_blue` value spikes.
* **Action:** Adjust the final global multiplier in the code so this yellow stack outputs a visibly high, "dangerous" reading (e.g., `15.0 mg/dL`).

---

## 6. Software Execution & Data Flow

### ESP32 Firmware State Machine
1. **Boot:** Initializes SPI TFT, draws the BEAM logo.
2. **Sensor Check:** Boots I2C. If the AS7343 is missing or covered by factory tape (yielding exactly `4.82` absorbance), halts with an error.
3. **WiFi/OTA:** Attempts connection. If successful, initializes OTA for wireless flashing and HTTPClient.
4. **Standby:** Displays a visual heartbeat. Constantly reads the button pin (GPIO 27) with a 50ms software debounce.
5. **Measurement Cycle:** * Fires the Cold White LED (GPIO 26).
   * Delays 500ms for light stabilization.
   * Triggers `readSpectraDataFromSensor()`.
   * Calculates Math.
   * Updates History Array.
   * Fires an HTTP POST request with JSON payload.
   * Updates TFT.

### Next.js API Infrastructure
The hardware targets a Next.js API route (`/api/readings`).
```json
// POST Payload Structure
{
  "b_abs": 1.25,
  "r_abs": 0.45,
  "tcb": 14.2
}
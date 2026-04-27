#include <WiFi.h>
#include <WiFiMulti.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SparkFun_AS7343.h>
#include <HTTPClient.h>

#define TFT_CS     5
#define TFT_RST    4 
#define TFT_DC     2 
#define LED_PIN    26
#define BTN_PIN    27

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
WiFiMulti wifiMulti;
SfeAS7343ArdI2C sensor; 

unsigned long last_heartbeat = 0;
bool heartbeat_state = false;
bool sensorReady = false;

float scanHistory[3] = {-1.0, -1.0, -1.0};

// ── Calibration state ─────────────────────────────────────────────────────────
// ref_blue / ref_red hold the white-paper baseline.
// hasReference = false → next button press does a calibration scan (white paper).
// hasReference = true  → next button press does a bilirubin measurement.
float ref_blue = 0.0;
float ref_red  = 0.0;
bool  hasReference = false;

// ── AS7343 channel helpers ────────────────────────────────────────────────────
// The AS7343 is an 18-channel spectral sensor.  getBlue()/getRed() do not exist
// on this chip and always return 0.  We must read named spectral channels.
//
// Channel map (SparkFun library, AUTOSMUX_18_CHANNELS mode):
//   getCh0()  → F1  ~405 nm  (violet)
//   getCh1()  → F2  ~425 nm  (violet-blue)   ← closest to bilirubin peak ~460nm
//   getCh2()  → F3  ~450 nm  (blue)           ← USE THIS for blue absorption
//   getCh3()  → F4  ~515 nm  (cyan-green)
//   getCh4()  → F5  ~555 nm  (green)
//   getCh5()  → F6  ~590 nm  (amber)
//   getCh6()  → F7  ~630 nm  (orange-red)
//   getCh7()  → F8  ~680 nm  (red)            ← USE THIS for red reference
//   getCh8()  → NIR ~910 nm
//   getCh9()  → CLEAR (broadband)
//   getCh10() → DARK (offset)
//   ... (remaining are duplicates / FD channels)
//
// Bilirubin absorbs strongly at ~460 nm and is transparent at ~680 nm.
// We measure how much blue is lost compared to our white-paper baseline,
// then subtract any red drift to cancel LED/geometry noise.
//
// If your SparkFun library version uses different method names, check:
//   sensor.getF1(), sensor.getF2() … sensor.getF8()
// Both naming schemes refer to the same physical channels.

inline float readBlueChannel() {
  // F3 ~450 nm — sits right on the bilirubin absorption peak
  return (float) sensor.getCh2();
}

inline float readRedChannel() {
  // F8 ~680 nm — bilirubin is optically transparent here; used as reference
  return (float) sensor.getCh7();
}
// ─────────────────────────────────────────────────────────────────────────────

void drawTopBar() {
  tft.fillRect(0, 0, 160, 15, ST77XX_BLACK); 
  tft.drawLine(0, 16, 160, 16, ST77XX_WHITE); 
  
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(2, 4);
  tft.print("Prev: ");
  for(int i=0; i<3; i++) {
    if(scanHistory[i] >= 0) {
      tft.print(scanHistory[i], 1);
    } else {
      tft.print("--");
    }
    if(i < 2) tft.print(" ");
  }

  if (WiFi.status() == WL_CONNECTED) {
    tft.fillRect(144, 10, 2, 2, ST77XX_GREEN);
    tft.fillRect(148, 8,  2, 4, ST77XX_GREEN);
    tft.fillRect(152, 6,  2, 6, ST77XX_GREEN);
    tft.fillRect(156, 4,  2, 8, ST77XX_GREEN);
  }
}

void drawReadyScreen() {
  tft.fillScreen(ST77XX_BLACK);
  drawTopBar();
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 50);

  if (!hasReference) {
    // Prompt user to calibrate first
    tft.setTextColor(ST77XX_YELLOW);
    tft.println("Place WHITE paper");
    tft.setCursor(10, 62);
    tft.println("then press button");
    tft.setCursor(10, 74);
    tft.setTextColor(ST77XX_CYAN);
    tft.println("to calibrate");
  } else {
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Ready for Scan");
    tft.setCursor(10, 65);
    tft.setTextColor(ST77XX_CYAN);
    tft.println("Press button to begin");
  }
}

// ── Raw channel read (LED on, stabilise, read, LED off) ───────────────────────
// Returns false if sensor.readSpectraDataFromSensor() fails.
bool readRawChannels(float &outBlue, float &outRed) {
  digitalWrite(LED_PIN, HIGH);
  delay(300);  // let LED reach steady state before triggering integration

  bool ok = sensor.readSpectraDataFromSensor();

  digitalWrite(LED_PIN, LOW);

  if (!ok) return false;

  outBlue = readBlueChannel();
  outRed  = readRedChannel();

  // Debug — always print to Serial so you can verify non-zero values
  Serial.printf("[RAW] Blue(~450nm): %.0f  Red(~680nm): %.0f\n", outBlue, outRed);

  return true;
}

// ── Calibration scan (white paper) ────────────────────────────────────────────
void doCalibration() {
  tft.fillScreen(ST77XX_BLACK);
  drawTopBar();
  tft.setCursor(10, 50);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("CALIBRATING");

  float b, r;
  if (!readRawChannels(b, r)) {
    tft.fillScreen(ST77XX_BLACK);
    drawTopBar();
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 50);
    tft.println("SENSOR ERR");
    delay(3000);
    drawReadyScreen();
    return;
  }

  // Guard against a dark/blocked sensor returning 0
  if (b < 10 || r < 10) {
    tft.fillScreen(ST77XX_BLACK);
    drawTopBar();
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(10, 50);
    tft.println("Readings too low!");
    tft.setCursor(10, 62);
    tft.println("Check LED & sensor tunnel");
    Serial.printf("[CAL FAIL] b=%.0f r=%.0f — values suspiciously low\n", b, r);
    delay(4000);
    drawReadyScreen();
    return;
  }

  ref_blue = b + 1.0;   // +1 prevents log10(0) if channel ever reads 0
  ref_red  = r + 1.0;
  hasReference = true;

  Serial.printf("[CAL OK]  ref_blue=%.1f  ref_red=%.1f\n", ref_blue, ref_red);

  tft.fillScreen(ST77XX_BLACK);
  drawTopBar();
  tft.setCursor(10, 35);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.println("Calibration OK!");
  tft.setCursor(10, 50);
  tft.setTextColor(ST77XX_WHITE);
  tft.printf("Ref B: %.0f", ref_blue);
  tft.setCursor(10, 62);
  tft.printf("Ref R: %.0f", ref_red);
  tft.setCursor(10, 80);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("Now place sample &");
  tft.setCursor(10, 92);
  tft.println("press button to scan");

  delay(3000);
  drawReadyScreen();
}

// ── Bilirubin measurement scan ────────────────────────────────────────────────
void doMeasurement() {
  tft.fillScreen(ST77XX_BLACK);
  drawTopBar();
  tft.setCursor(10, 50);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("SCANNING...");

  float blueRaw, redRaw;
  if (!readRawChannels(blueRaw, redRaw)) {
    tft.fillScreen(ST77XX_BLACK);
    drawTopBar();
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 50);
    tft.println("SENSOR ERR");
    delay(3000);
    drawReadyScreen();
    return;
  }

  // Beer-Lambert absorbance relative to white-paper baseline:
  //   A = log10(I_reference / I_sample)
  //   Positive A → sample absorbed that wavelength compared to white paper.
  //
  // A_blue  > 0 when blue is absorbed (bilirubin present, or yellow pigment).
  // A_red  ≈ 0 when nothing absorbs red (bilirubin is transparent at 680nm).
  //
  // Differential index = A_blue - A_red cancels:
  //   • ambient light leakage (affects both channels equally)
  //   • LED intensity drift  (affects both channels equally)
  // leaving only bilirubin-specific blue absorption.

  float A_blue = log10(ref_blue / (blueRaw + 1.0));
  float A_red  = log10(ref_red  / (redRaw  + 1.0));
  float bilirubinIndex = A_blue - A_red;

  if (bilirubinIndex < 0) bilirubinIndex = 0;

  // Empirical scale factor k: maps the dimensionless index → mg/dL.
  // k = 15 is a reasonable starting point; calibrate against a known
  // bilirubin standard or a clinical bilirubinometer to refine it.
  const float k = 15.0;
  float estimatedTcB = bilirubinIndex * k;

  Serial.printf("[RESULT] A_blue=%.4f  A_red=%.4f  index=%.4f  TcB=%.2f mg/dL\n",
                A_blue, A_red, bilirubinIndex, estimatedTcB);

  // Shift history
  scanHistory[2] = scanHistory[1];
  scanHistory[1] = scanHistory[0];
  scanHistory[0] = estimatedTcB;

  // POST to Vercel endpoint if connected
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://your-vercel-app-url.vercel.app/api/readings");
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"b_raw\":" + String(blueRaw, 0)
                   + ",\"r_raw\":"  + String(redRaw,  0)
                   + ",\"a_blue\":" + String(A_blue,  4)
                   + ",\"a_red\":"  + String(A_red,   4)
                   + ",\"index\":"  + String(bilirubinIndex, 4)
                   + ",\"tcb\":"    + String(estimatedTcB, 2) + "}";
    int httpCode = http.POST(payload);
    Serial.printf("[HTTP] POST → %d\n", httpCode);
    http.end();
  }

  // ── Display results ──────────────────────────────────────────────────────
  tft.fillScreen(ST77XX_BLACK);
  drawTopBar();

  tft.setCursor(10, 22);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.printf("B(450): %.0f  A=%.3f", blueRaw, A_blue);

  tft.setCursor(10, 34);
  tft.setTextColor(ST77XX_RED);
  tft.printf("R(680): %.0f  A=%.3f", redRaw, A_red);

  tft.setCursor(10, 46);
  tft.setTextColor(ST77XX_WHITE);
  tft.printf("Index: %.4f", bilirubinIndex);

  tft.setCursor(20, 65);
  tft.setTextSize(4);
  tft.setTextColor(ST77XX_YELLOW);
  tft.printf("%.1f", estimatedTcB);

  tft.setTextSize(1);
  tft.setCursor(20, 110);
  tft.println("mg/dL (Bilirubin)");

  delay(6000);
  drawReadyScreen();
}

// =============================================================================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);

  tft.initR(INITR_GREENTAB); 
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  
  // ── Splash screen (unchanged) ──────────────────────────────────────────────
  tft.setTextSize(4);
  tft.setCursor(35, 25);
  tft.setTextColor(ST77XX_CYAN);    tft.print("B");
  tft.setTextColor(ST77XX_MAGENTA); tft.print("E");
  tft.setTextColor(ST77XX_YELLOW);  tft.print("A");
  tft.setTextColor(ST77XX_GREEN);   tft.print("M");
  
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, 65);
  tft.println("Bilirubin Evaluation");
  tft.setCursor(25, 80);
  tft.println("and Analysis Meter");
  delay(3000); 

  // ── Sensor init ───────────────────────────────────────────────────────────
  Wire.begin(21, 22); 
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  
  if (sensor.begin(0x39, Wire) == false) {
    tft.setTextColor(ST77XX_RED);
    tft.println("Sensor: NOT FOUND");
    sensorReady = false;
  } else {
    sensor.powerOn();
    sensor.setAutoSmux(AUTOSMUX_18_CHANNELS);
    sensor.enableSpectralMeasurement();

    tft.setTextColor(ST77XX_GREEN);
    tft.println("Sensor: OK");
    sensorReady = true;
  }

  // ── WiFi (unchanged) ──────────────────────────────────────────────────────
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Connecting WiFi...");
  wifiMulti.addAP("WPA2-Home", "your_wifi_password");
  wifiMulti.addAP("WPA2-Office", "your_wifi_password");

  int attempts = 0;
  while (wifiMulti.run() != WL_CONNECTED && attempts < 6) {
    delay(500);
    tft.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    tft.println("\nWiFi: Connected");
    
    ArduinoOTA.onStart([]() {
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_MAGENTA);
      tft.setTextSize(2);
      tft.setCursor(10, 40);
      tft.println("UPDATING...");
    });
    ArduinoOTA.onEnd([]() {
      tft.fillScreen(ST77XX_BLUE);
      tft.setCursor(10, 50);
      tft.setTextColor(ST77XX_WHITE);
      tft.println("UPDATE DONE!");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      int percentage = (progress / (total / 100));
      tft.fillRect(10, 80, 140, 10, ST77XX_BLACK); 
      tft.drawRect(10, 80, 140, 10, ST77XX_WHITE); 
      tft.fillRect(10, 80, (1.4 * percentage), 10, ST77XX_GREEN); 
    });
    ArduinoOTA.setPassword("kris");
    ArduinoOTA.begin();
  } else {
    tft.setTextColor(ST77XX_YELLOW);
    tft.println("\nWiFi: Offline Mode");
  }

  delay(1500);
  drawReadyScreen();  // will prompt "Place WHITE paper" since hasReference=false
}

// =============================================================================
void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
  }

  // Heartbeat dot (unchanged)
  if (millis() - last_heartbeat > 1000) {
    last_heartbeat = millis();
    heartbeat_state = !heartbeat_state;
    tft.fillCircle(150, 120, 2, heartbeat_state ? ST77XX_GREEN : ST77XX_BLACK);
  }

  // Button with debounce
  if (digitalRead(BTN_PIN) == LOW) {
    delay(50);
    if (digitalRead(BTN_PIN) == LOW) {

      if (!sensorReady) {
        // Sensor never initialised — show error and bail
        tft.fillScreen(ST77XX_BLACK);
        drawTopBar();
        tft.setTextColor(ST77XX_RED);
        tft.setTextSize(1);
        tft.setCursor(10, 50);
        tft.println("Sensor not ready");
        delay(2000);
        drawReadyScreen();
        return;
      }

      if (!hasReference) {
        // ── First press: calibrate with white paper ──────────────────────
        doCalibration();
      } else {
        // ── Subsequent presses: measure bilirubin ────────────────────────
        doMeasurement();
      }

      // Wait for button release before returning to loop
      while (digitalRead(BTN_PIN) == LOW) delay(10);
    }
  }
}

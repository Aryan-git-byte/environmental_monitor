/* Environmental Monitor - Modern UI Design
   - BMP280 (I2C: SDA=21, SCL=22)
   - DHT11 (GPIO27)
   - MQ135 (AO -> divider -> GPIO33), heater 5V
   - Optional CAL button (GPIO14 active LOW)
   - Modern card-based UI with status indicators
*/

#include <TFT_eSPI.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <Preferences.h>
#include <math.h>

TFT_eSPI tft = TFT_eSPI();
Adafruit_BMP280 bmp;
DHT dht(27, DHT11);
Preferences prefs;

// ----- MQ135 config -----
#define MQ_ADC_PIN 33
#define MQ_RL 22000.0
#define MQ_VCC 5.0
#define DIVIDER_RATIO 0.6666667
#define RO_CLEAN_AIR_FACTOR 3.6
#define R0_PRECALIBRATED 76000.0  // Pre-calibrated R0 value (adjust based on your sensor)

// ----- Cal button -----
#define CAL_PIN 14

// ----- UI config -----
#define SCREEN_W 320
#define SCREEN_H 240
#define GRAPH_LEN 60

// Custom colors for modern UI
#define COLOR_BG 0x0841        // Dark blue-gray background
#define COLOR_CARD 0x18E3      // Card background
#define COLOR_ACCENT 0x05FF    // Cyan accent
#define COLOR_GOOD 0x07E0      // Green
#define COLOR_WARNING 0xFD20   // Orange
#define COLOR_DANGER 0xF800    // Red
#define COLOR_TEXT_DIM 0x7BEF  // Dimmed text

// Rolling buffers
float g_co2[GRAPH_LEN];
float g_temp[GRAPH_LEN];
float g_hum[GRAPH_LEN];
float g_pres[GRAPH_LEN];
int g_index = 0;

float R0 = R0_PRECALIBRATED;  // Use pre-calibrated value

// Forward declarations
float readMQppm();
float readRs();
void calibrateR0();
void drawModernUI(float bmpT, float pres, float dhtT, float hum, float co2);
void drawCard(int x, int y, int w, int h, const char* title, float value, const char* unit, uint16_t color);
void drawMiniGraph(int x, int y, int w, int h, float* data, uint16_t color, float maxVal);
void drawCO2Status(int x, int y, float co2);
uint16_t getCO2Color(float co2);

void setup() {
  Serial.begin(115200);
  delay(200);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);

  Wire.begin(21, 22);

  if (!bmp.begin(0x76)) bmp.begin(0x77);
  dht.begin();

  pinMode(CAL_PIN, INPUT_PULLUP);

  prefs.begin("envmon", false);
  
  // Load saved R0 or use pre-calibrated value
  float savedR0 = prefs.getFloat("mq_r0", 0.0);
  if (savedR0 > 0.001) {
    R0 = savedR0;  // Use saved calibration
  }
  // else R0 already set to R0_PRECALIBRATED

  // Splash screen
  tft.fillScreen(COLOR_BG);
  tft.setTextSize(3);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(30, 80);
  tft.print("ENV MONITOR");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_DIM);
  tft.setCursor(60, 120);
  tft.print("Initializing sensors...");

  delay(1000);

  // Simplified status check (no warning needed with pre-calibration)
  tft.fillRect(60, 150, 200, 20, COLOR_BG);
  tft.setTextColor(COLOR_GOOD);
  tft.setTextSize(1);
  tft.setCursor(80, 155);
  tft.print("MQ135 Ready");
  delay(1000);

  if (digitalRead(CAL_PIN) == LOW) {
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(40, 100);
    tft.print("CALIBRATING...");
    tft.setTextSize(1);
    tft.setCursor(60, 130);
    tft.print("Keep in clean air");
    calibrateR0();
  }

  for (int i = 0; i < GRAPH_LEN; i++) {
    g_co2[i] = 0; g_temp[i] = 0; g_hum[i] = 0; g_pres[i] = 0;
  }

  tft.fillScreen(COLOR_BG);
}

unsigned long lastUI = 0;
void loop() {
  float bmpTemp = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0;
  float dhtTemp = dht.readTemperature();
  float hum = dht.readHumidity();
  float co2 = readMQppm();

  g_co2[g_index] = co2;
  g_temp[g_index] = (bmpTemp + dhtTemp) / 2.0; // Average temp
  g_hum[g_index] = hum;
  g_pres[g_index] = pressure;
  g_index = (g_index + 1) % GRAPH_LEN;

  if (millis() - lastUI > 1000) {
    drawModernUI(bmpTemp, pressure, dhtTemp, hum, co2);
    lastUI = millis();
  }

  // Calibration trigger
  static unsigned long calPressedAt = 0;
  if (digitalRead(CAL_PIN) == LOW) {
    if (calPressedAt == 0) calPressedAt = millis();
    if (millis() - calPressedAt > 3000) {
      tft.fillScreen(COLOR_BG);
      tft.setTextSize(2);
      tft.setTextColor(COLOR_WARNING);
      tft.setCursor(40, 100);
      tft.print("CALIBRATING...");
      calibrateR0();
      tft.fillScreen(COLOR_BG);
      calPressedAt = 0;
    }
  } else calPressedAt = 0;

  delay(200);
}

// ---------- MQ helpers ----------
float readMQppm() {
  float rs = readRs();
  if (rs <= 0.0) return 400.0;  // Return baseline value on error
  float ratio = rs / R0;
  float ppm = 116.6020682 * pow(ratio, -2.769034857);
  if (!isfinite(ppm) || ppm < 0) ppm = 400.0;  // Default to baseline
  if (ppm > 5000) ppm = 5000;  // Cap at reasonable max
  return ppm;
}

float readRs() {
  int adc = analogRead(MQ_ADC_PIN);
  if (adc <= 0) return 999999.0;
  float v_adc = (adc / 4095.0) * 3.3;
  float v_out = v_adc / DIVIDER_RATIO;
  if (v_out <= 0.001) return 999999.0;
  float rs = MQ_RL * (MQ_VCC - v_out) / v_out;
  return rs;
}

void calibrateR0() {
  const int samples = 200;
  double rs_accum = 0.0;
  for (int i = 0; i < samples; i++) {
    rs_accum += readRs();
    delay(300);
  }
  double rs_avg = rs_accum / samples;
  R0 = rs_avg / RO_CLEAN_AIR_FACTOR;
  prefs.putFloat("mq_r0", R0);
}

// ---------- Modern UI ----------
void drawModernUI(float bmpT, float pres, float dhtT, float hum, float co2) {
  tft.fillScreen(COLOR_BG);

  // Header bar with gradient effect
  for (int i = 0; i < 24; i++) {
    uint16_t c = tft.color565(0, 90 - i*2, 130 - i*2);
    tft.drawFastHLine(0, i, SCREEN_W, c);
  }
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(8, 4);
  tft.print("ENVIRONMENT");
  
  // Status indicator
  int statusX = SCREEN_W - 50;
  uint16_t statusColor = COLOR_GOOD;  // Always green with pre-calibration
  tft.fillCircle(statusX, 12, 6, statusColor);
  tft.setTextSize(1);
  tft.setCursor(statusX + 10, 8);
  tft.setTextColor(COLOR_TEXT_DIM);
  tft.print("OK");

  // CO2 Hero Card (large, centered top)
  int heroX = 10, heroY = 30, heroW = 300, heroH = 60;
  tft.fillRoundRect(heroX, heroY, heroW, heroH, 8, COLOR_CARD);
  
  // CO2 status ring
  drawCO2Status(heroX + 30, heroY + 30, co2);
  
  // CO2 value
  tft.setTextSize(3);
  tft.setTextColor(getCO2Color(co2));
  tft.setCursor(heroX + 70, heroY + 12);
  tft.print((int)co2);
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_DIM);
  tft.setCursor(heroX + 70, heroY + 42);
  tft.print("CO2 ppm");
  
  // Air quality label
  tft.setTextSize(1);
  tft.setCursor(heroX + 180, heroY + 20);
  if (co2 < 400) {
    tft.setTextColor(COLOR_GOOD);
    tft.print("EXCELLENT");
  } else if (co2 < 1000) {
    tft.setTextColor(COLOR_GOOD);
    tft.print("GOOD");
  } else if (co2 < 2000) {
    tft.setTextColor(COLOR_WARNING);
    tft.print("MODERATE");
  } else {
    tft.setTextColor(COLOR_DANGER);
    tft.print("POOR");
  }

  // Mini graph in CO2 card
  drawMiniGraph(heroX + 170, heroY + 35, 120, 20, g_co2, getCO2Color(co2), 2000);

  // Metric cards (2x2 grid)
  int cardY = 100;
  int cardW = 145, cardH = 65;
  int gap = 10;

  // Temperature card (average)
  float avgTemp = (bmpT + dhtT) / 2.0;
  drawCard(10, cardY, cardW, cardH, "TEMPERATURE", avgTemp, "C", COLOR_ACCENT);
  drawMiniGraph(15, cardY + 38, cardW - 10, 22, g_temp, COLOR_ACCENT, 50);

  // Humidity card
  drawCard(165, cardY, cardW, cardH, "HUMIDITY", hum, "%", 0x07FF);
  drawMiniGraph(170, cardY + 38, cardW - 10, 22, g_hum, 0x07FF, 100);

  // Pressure card
  drawCard(10, cardY + 75, cardW, cardH, "PRESSURE", pres, "hPa", 0xFBE0);
  drawMiniGraph(15, cardY + 113, cardW - 10, 22, g_pres, 0xFBE0, 1100);

  // Sensor info card
  tft.fillRoundRect(165, cardY + 75, cardW, cardH, 6, COLOR_CARD);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_DIM);
  tft.setCursor(170, cardY + 80);
  tft.print("SENSORS");
  
  tft.setTextSize(1);
  tft.setCursor(170, cardY + 95);
  tft.setTextColor(COLOR_GOOD);
  tft.print("BMP: ");
  tft.setTextColor(TFT_WHITE);
  tft.print(bmpT, 1);
  tft.print("C");
  
  tft.setCursor(170, cardY + 108);
  tft.setTextColor(COLOR_GOOD);
  tft.print("DHT: ");
  tft.setTextColor(TFT_WHITE);
  tft.print(dhtT, 1);
  tft.print("C");
  
  tft.setCursor(170, cardY + 121);
  tft.setTextColor(COLOR_GOOD);
  tft.print("MQ135: ");
  tft.setTextColor(COLOR_GOOD);
  tft.print("READY");
}

void drawCard(int x, int y, int w, int h, const char* title, float value, const char* unit, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, 6, COLOR_CARD);
  
  // Title
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_DIM);
  tft.setCursor(x + 5, y + 5);
  tft.print(title);
  
  // Value
  tft.setTextSize(2);
  tft.setTextColor(color);
  tft.setCursor(x + 5, y + 18);
  
  char buf[16];
  if (value < 10) {
    sprintf(buf, "%.1f", value);
  } else {
    sprintf(buf, "%.0f", value);
  }
  tft.print(buf);
  
  // Unit
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_DIM);
  tft.print(" ");
  tft.print(unit);
}

void drawMiniGraph(int x, int y, int w, int h, float* data, uint16_t color, float maxVal) {
  // Draw subtle background
  tft.drawRect(x, y, w, h, COLOR_TEXT_DIM);
  
  // Draw graph line
  for (int i = 0; i < GRAPH_LEN - 1; i++) {
    int idx1 = (g_index + i) % GRAPH_LEN;
    int idx2 = (g_index + i + 1) % GRAPH_LEN;
    float v1 = data[idx1];
    float v2 = data[idx2];
    
    if (!isfinite(v1)) v1 = 0;
    if (!isfinite(v2)) v2 = 0;
    
    int x1 = x + (i * w) / GRAPH_LEN;
    int x2 = x + ((i + 1) * w) / GRAPH_LEN;
    int y1 = y + h - (int)((v1 / maxVal) * h);
    int y2 = y + h - (int)((v2 / maxVal) * h);
    
    // Clamp values
    y1 = constrain(y1, y, y + h);
    y2 = constrain(y2, y, y + h);
    
    tft.drawLine(x1, y1, x2, y2, color);
  }
}

void drawCO2Status(int cx, int cy, float co2) {
  // Draw status ring
  uint16_t color = getCO2Color(co2);
  
  // Outer ring
  for (int r = 18; r <= 20; r++) {
    tft.drawCircle(cx, cy, r, color);
  }
  
  // Inner fill (pulsing effect would need animation)
  tft.fillCircle(cx, cy, 16, COLOR_CARD);
  
  // Icon/indicator
  tft.fillCircle(cx, cy, 8, color);
}

uint16_t getCO2Color(float co2) {
  if (co2 < 800) return COLOR_GOOD;
  if (co2 < 1500) return COLOR_WARNING;
  return COLOR_DANGER;
}
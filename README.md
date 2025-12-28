# ESP32 Environmental Monitor

A modern, visually appealing environmental monitoring system using ESP32 with multiple sensors and a TFT display.

![Project Status](https://img.shields.io/badge/status-active-success.svg)
![Arduino](https://img.shields.io/badge/Arduino-Compatible-blue.svg)
![ESP32](https://img.shields.io/badge/ESP32-Compatible-green.svg)

## 📋 Table of Contents
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Wiring Diagram](#wiring-diagram)
- [Software Setup](#software-setup)
- [TFT_eSPI Configuration](#tft_espi-configuration)
- [Installation](#installation)
- [Usage](#usage)
- [Calibration](#calibration)
- [Troubleshooting](#troubleshooting)

---

## ✨ Features

- **Real-time monitoring** of:
  - Temperature (BMP280 & DHT11)
  - Humidity (DHT11)
  - Atmospheric Pressure (BMP280)
  - CO2 equivalent levels (MQ135)
  
- **Modern Dashboard UI**:
  - Card-based layout
  - Color-coded status indicators
  - Real-time trend graphs
  - Air quality status display

- **Data Visualization**:
  - Rolling graph buffers (60 samples)
  - Mini sparklines for each metric
  - Large CO2 hero display

- **Smart Features**:
  - Pre-calibrated MQ135 sensor
  - Optional manual calibration via button
  - Persistent storage of calibration data
  - Color-coded air quality warnings

---

## 🛠️ Hardware Requirements

### Components List

| Component | Model | Quantity | Notes |
|-----------|-------|----------|-------|
| Microcontroller | ESP32 DevKit | 1 | Any ESP32 board with I2C & ADC |
| Display | ILI9341 2.4" TFT | 1 | 320x240 SPI display |
| Temperature/Pressure | BMP280 | 1 | I2C sensor |
| Temperature/Humidity | DHT11 | 1 | Digital sensor |
| Air Quality | MQ135 | 1 | Analog gas sensor |
| Resistors | 10kΩ | 1 | For voltage divider |
| Resistors | 20kΩ | 1 | For voltage divider |
| Button | Tactile switch | 1 | Optional calibration |
| Breadboard | - | 1 | For prototyping |
| Jumper Wires | - | ~20 | Mixed M-M, M-F |

### Power Requirements
- **ESP32**: 5V via USB or 3.3V regulated
- **MQ135 Heater**: Requires 5V supply
- **Total Current**: ~300-500mA (with MQ135 active)

---

## 🔌 Wiring Diagram

### Pin Connections

#### TFT Display (ILI9341) - SPI Interface
```
TFT Pin    →   ESP32 Pin
─────────────────────────
VCC        →   3.3V
GND        →   GND
CS         →   GPIO 15
RESET      →   GPIO 4
DC/RS      →   GPIO 2
MOSI       →   GPIO 23
SCK        →   GPIO 18
MISO       →   GPIO 19
LED        →   3.3V (backlight)
T_CS       →   GPIO 21 (touch, optional)
```

#### BMP280 - I2C Interface
```
BMP280 Pin →   ESP32 Pin
─────────────────────────
VCC        →   3.3V
GND        →   GND
SDA        →   GPIO 21
SCL        →   GPIO 22
```

#### DHT11 - Digital Interface
```
DHT11 Pin  →   ESP32 Pin
─────────────────────────
VCC        →   3.3V or 5V
GND        →   GND
DATA       →   GPIO 27
```

#### MQ135 - Analog with Voltage Divider
```
MQ135 Pin  →   Connection
─────────────────────────────
VCC        →   5V (heater power)
GND        →   GND
AO         →   10kΩ resistor → GPIO 33
           →   20kΩ resistor → GND
```

**⚠️ CRITICAL: Voltage Divider for MQ135**
```
MQ135 AO (0-5V) ──┬── 10kΩ ──┬── GPIO 33 (ADC)
                  │          │
                  │          └── 20kΩ ── GND
                  │
              (Direct from AO)
```

This creates a voltage divider ratio of 0.6667 (20k/(10k+20k))
- Input: 0-5V from MQ135
- Output: 0-3.3V safe for ESP32 ADC

#### Calibration Button (Optional)
```
Button Pin →   ESP32 Pin
─────────────────────────
Pin 1      →   GPIO 14
Pin 2      →   GND
```
Note: Uses internal pull-up resistor

### Complete Wiring Schematic

```
                    ESP32
                 ┌─────────┐
    5V ──────────┤ VIN  23 ├──────── TFT MOSI
    GND ─────────┤ GND  18 ├──────── TFT SCK
                 │      19 ├──────── TFT MISO
    TFT CS ──────┤ 15    4 ├──────── TFT RST
    TFT DC ──────┤ 2    21 ├──────── BMP280 SDA / TFT Touch CS
                 │      22 ├──────── BMP280 SCL
    DHT11 ───────┤ 27   33 ├──────── MQ135 (via divider)
    CAL BTN ─────┤ 14      │
                 └─────────┘

    BMP280           DHT11          MQ135
    ┌──────┐       ┌──────┐      ┌────────┐
    │ VCC──┼───3.3V│ VCC──┼──3.3V│ VCC────┼──5V
    │ GND──┼───GND │ GND──┼──GND │ GND────┼──GND
    │ SDA──┼───21  │ DATA─┼──27  │ AO─────┼──Divider──33
    │ SCL──┼───22  └──────┘      └────────┘
    └──────┘
```

---

## 💻 Software Setup

### Arduino IDE Configuration

1. **Install ESP32 Board Support**
   - Open Arduino IDE
   - Go to `File` → `Preferences`
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to `Tools` → `Board` → `Boards Manager`
   - Search for "ESP32" and install

2. **Select Board**
   - `Tools` → `Board` → `ESP32 Arduino` → `ESP32 Dev Module`
   - Set Upload Speed: `115200`
   - Set Flash Frequency: `80MHz`
   - Set Partition Scheme: `Default 4MB with spiffs`

### Required Libraries

Install these libraries via Arduino Library Manager (`Sketch` → `Include Library` → `Manage Libraries`):

| Library | Version | Purpose |
|---------|---------|---------|
| TFT_eSPI | 2.5.0+ | TFT display driver |
| Adafruit BMP280 | 2.6.0+ | BMP280 sensor |
| DHT sensor library | 1.4.0+ | DHT11 sensor |
| Adafruit Unified Sensor | 1.1.0+ | Required by DHT |
| Preferences | Built-in | ESP32 storage |

**Installation Steps:**
```
1. Sketch → Include Library → Manage Libraries
2. Search for each library name
3. Click "Install"
4. Repeat for all libraries
```

---

## ⚙️ TFT_eSPI Configuration

### Critical Setup File

The TFT_eSPI library requires manual configuration. You must edit the `User_Setup.h` file.

**File Location:**
- Windows: `Documents\Arduino\libraries\TFT_eSPI\User_Setup.h`
- Mac: `~/Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`
- Linux: `~/Arduino/libraries/TFT_eSPI/User_Setup.h`

### Complete User_Setup.h Configuration

**Replace the entire contents** of `User_Setup.h` with:

```cpp
// ===============================================
// USER_SETUP.H FOR ESP32 + ILI9341 2.4" TFT
// ===============================================

#define USER_SETUP_ID 42

// ===== DRIVER SELECTION =====
#define ILI9341_2_DRIVER      // ⭐ Critical: Use ILI9341_2_DRIVER (not ILI9341_DRIVER)

// ===== ESP32 SPI PINS =====
#define TFT_MISO 19           // Master In Slave Out
#define TFT_MOSI 23           // Master Out Slave In
#define TFT_SCLK 18           // Serial Clock
#define TFT_CS   15           // Chip Select
#define TFT_DC    2           // Data/Command
#define TFT_RST   4           // Reset

// ===== DISPLAY SETTINGS =====
#define TFT_ROTATION 0        // 0 = Portrait, 1 = Landscape, 2 = Inverted Portrait, 3 = Inverted Landscape

// ===== SPI SPEEDS (STABLE) =====
#define SPI_FREQUENCY       27000000   // 27 MHz (stable for most displays)
#define SPI_READ_FREQUENCY  16000000   // 16 MHz for reading
#define SPI_TOUCH_FREQUENCY  2500000   // 2.5 MHz for touch (if used)

// ===== DISPLAY INVERSION =====
#define TFT_INVERT_ON_START            // Some ILI9341 panels need inverted colors

// ===== FONT LOADING =====
#define LOAD_GLCD              // Font 1: Original Adafruit 8 pixel font
#define LOAD_FONT2             // Font 2: Small 16 pixel high font
#define LOAD_FONT4             // Font 4: Medium 26 pixel high font
#define LOAD_GFXFF             // FreeFonts
#define SMOOTH_FONT            // Enable anti-aliased fonts

// ===== TOUCH SCREEN (OPTIONAL) =====
#define TOUCH_CS 21                    // Touch chip select
// Uncomment if using resistive touch:
// #define SUPPORT_TRANSACTIONS
```

### Alternative: Using Setup File Method

If you prefer not to modify `User_Setup.h`, you can use a custom setup file:

1. Create `User_Setup_Custom.h` in your project folder
2. Copy the configuration above into it
3. In `User_Setup.h`, comment out all other configurations and add:
   ```cpp
   #include <User_Setup_Custom.h>
   ```

### Verify Configuration

After editing, verify by compiling a simple test:

```cpp
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  tft.fillScreen(TFT_RED);
  delay(1000);
  tft.fillScreen(TFT_GREEN);
  delay(1000);
  tft.fillScreen(TFT_BLUE);
}

void loop() {}
```

If you see red → green → blue, configuration is correct!

---

## 📥 Installation

### Step 1: Clone/Download Code

Download the main code file (provided in the artifact above).

### Step 2: Open in Arduino IDE

1. Open `environmental_monitor.ino` in Arduino IDE
2. Verify all libraries are installed (see [Software Setup](#software-setup))

### Step 3: Configure TFT_eSPI

Follow the [TFT_eSPI Configuration](#tft_espi-configuration) section carefully.

### Step 4: Adjust MQ135 Calibration (Optional)

In the code, find this line:
```cpp
#define R0_PRECALIBRATED 76000.0  // Pre-calibrated R0 value
```

Adjust based on your environment:
- **Typical range**: 50,000 - 100,000 Ω
- **Urban environment**: Use lower values (50k-70k)
- **Clean air**: Use higher values (80k-100k)

### Step 5: Upload Code

1. Connect ESP32 via USB
2. Select correct COM port: `Tools` → `Port`
3. Click Upload button (→)
4. Wait for "Done uploading" message

### Step 6: Power Cycle

After first upload:
1. Disconnect USB
2. Wait 5 seconds
3. Reconnect USB
4. Monitor display startup

---

## 🚀 Usage

### Normal Operation

1. **Power On**: Display shows splash screen for 2 seconds
2. **Sensor Initialization**: All sensors initialize automatically
3. **Main Dashboard**: Modern UI appears with:
   - Large CO2 reading at top
   - Temperature card (averaged BMP280 & DHT11)
   - Humidity card with sparkline
   - Pressure card with trend
   - Sensor status card

### Reading the Display

#### Header Bar
- **"ENVIRONMENT"** title
- **Status Dot** (top right):
  - 🟢 Green = All systems OK
  - 🟡 Yellow = Calibration needed (if modified)

#### CO2 Hero Card (Top)
- **Large Number**: Current CO2-equivalent in ppm
- **Color Coding**:
  - 🟢 Green (< 800 ppm): Excellent air quality
  - 🟡 Yellow (800-1500 ppm): Good, ventilation recommended
  - 🔴 Red (> 1500 ppm): Poor, ventilate immediately
- **Status Label**: "EXCELLENT" / "GOOD" / "MODERATE" / "POOR"
- **Mini Graph**: 60-second rolling trend

#### Metric Cards (Bottom Grid)

**Temperature Card** (Top Left)
- Averaged reading from BMP280 and DHT11
- Cyan color theme
- Rolling 60-second graph

**Humidity Card** (Top Right)
- DHT11 humidity percentage
- Cyan accent
- Sparkline trend

**Pressure Card** (Bottom Left)
- BMP280 atmospheric pressure in hPa
- Yellow theme
- Pressure trend graph

**Sensor Status Card** (Bottom Right)
- Individual sensor readouts
- BMP280 temperature
- DHT11 temperature
- MQ135 status

### Data Update Rate
- **Sensor Reading**: Every 200ms
- **Display Update**: Every 1 second
- **Graph Update**: Every second (60-point buffer)

---

## 🎯 Calibration

### Pre-Calibration (Default)

The sensor ships with `R0_PRECALIBRATED = 76000.0`. This works for most environments but may need adjustment.

### Manual Calibration (Recommended for Accuracy)

For best results, calibrate in clean outdoor air:

1. **Prepare Environment**:
   - Take device outdoors (away from traffic)
   - OR use a room with open windows after 30 minutes
   - Avoid: kitchens, garages, near people/vehicles

2. **Warm-up Period**:
   - Power on device
   - Wait 5-10 minutes minimum
   - Ideal: 24-48 hours powered on

3. **Calibrate**:
   - Hold CAL button (GPIO14) for **3+ seconds**
   - Display shows "CALIBRATING..."
   - Keep device in clean air for 60 seconds
   - Display shows "CAL DONE. R0=XXXXX (saved)"

4. **Verify**:
   - CO2 reading should be 380-450 ppm in clean outdoor air
   - If reading is off, adjust `R0_PRECALIBRATED` and re-upload

### Troubleshooting Calibration

**CO2 reads too high** (e.g., 800+ ppm in clean air):
- Increase `R0_PRECALIBRATED` value
- Example: Change from 76000 to 100000

**CO2 reads too low** (e.g., 200 ppm):
- Decrease `R0_PRECALIBRATED` value
- Example: Change from 76000 to 50000

**CO2 shows 400 ppm constantly**:
- Check MQ135 wiring
- Verify voltage divider connections
- Ensure 5V power to MQ135 VCC

---

## 🔧 Troubleshooting

### Display Issues

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| White screen | Wrong driver in User_Setup.h | Use `ILI9341_2_DRIVER` |
| No display | Wiring issue | Check all SPI connections |
| Inverted colors | Color inversion setting | Add/remove `TFT_INVERT_ON_START` |
| Flickering | SPI speed too high | Reduce to 27 MHz |
| Garbage display | Wrong pins | Verify User_Setup.h matches wiring |

### Sensor Issues

**BMP280 Not Found**
```
Solution:
1. Check I2C wiring (SDA=21, SCL=22)
2. Verify 3.3V power
3. Try alternate I2C address:
   if (!bmp.begin(0x76)) bmp.begin(0x77);
```

**DHT11 Returns NaN**
```
Solution:
1. Check DATA pin connection (GPIO 27)
2. Add 10kΩ pull-up resistor between DATA and VCC
3. Wait 2 seconds after power-on before reading
```

**MQ135 Shows 400 ppm Constantly**
```
Solution:
1. Verify voltage divider: 10kΩ and 20kΩ resistors
2. Check 5V power to MQ135 VCC (heater needs 5V!)
3. Wait 5-10 minutes for heater warm-up
4. Verify GPIO 33 is connected to divider midpoint
5. Run calibration in clean air
```

**MQ135 Reads Too High/Low**
```
Solution:
1. Re-calibrate in clean outdoor air
2. Adjust R0_PRECALIBRATED value in code
3. Ensure voltage divider ratio matches code (0.6667)
```

### Serial Debug

Add this to `loop()` for debugging:

```cpp
Serial.print("BMP Temp: "); Serial.println(bmpTemp);
Serial.print("DHT Temp: "); Serial.println(dhtTemp);
Serial.print("Humidity: "); Serial.println(hum);
Serial.print("Pressure: "); Serial.println(pressure);
Serial.print("MQ ADC: "); Serial.println(analogRead(33));
Serial.print("CO2: "); Serial.println(co2);
Serial.println("---");
```

Open Serial Monitor at 115200 baud to view real-time data.

---

## 📊 Technical Specifications

### Measurement Ranges

| Sensor | Parameter | Range | Accuracy |
|--------|-----------|-------|----------|
| BMP280 | Temperature | -40 to +85°C | ±1°C |
| BMP280 | Pressure | 300-1100 hPa | ±1 hPa |
| DHT11 | Temperature | 0 to 50°C | ±2°C |
| DHT11 | Humidity | 20-90% RH | ±5% |
| MQ135 | CO2 eq. | 10-10000 ppm | ±10% (after calibration) |

### Power Consumption

| Component | Current Draw | Notes |
|-----------|--------------|-------|
| ESP32 (active) | 80-160 mA | WiFi disabled |
| TFT Display | 20-50 mA | Backlight on |
| BMP280 | 0.7 mA | Normal mode |
| DHT11 | 0.5 mA | Average |
| MQ135 | 150 mA | Heater active |
| **Total** | **250-360 mA** | 5V supply required |

### Memory Usage

- **Flash**: ~350 KB (sketch)
- **SRAM**: ~45 KB (global variables + buffers)
- **Heap**: Dynamic allocation for TFT

---

## 🎨 UI Color Scheme

```cpp
// Background & Cards
COLOR_BG        0x0841   // Dark blue-gray #104185
COLOR_CARD      0x18E3   // Medium gray #18E3E3

// Status Colors
COLOR_ACCENT    0x05FF   // Cyan accent
COLOR_GOOD      0x07E0   // Green (good readings)
COLOR_WARNING   0xFD20   // Orange (moderate)
COLOR_DANGER    0xF800   // Red (poor)
COLOR_TEXT_DIM  0x7BEF   // Dimmed text
```

---

## 📁 Project Structure

```
environmental_monitor/
├── environmental_monitor.ino    # Main Arduino sketch
├── README.md                    # This documentation
└── wiring_diagram.png           # Visual wiring guide (optional)
```

---

## 🔮 Future Enhancements

- [ ] WiFi connectivity for cloud logging
- [ ] MQTT integration for home automation
- [ ] SD card data logging
- [ ] Multiple MQ sensor support (MQ-7, MQ-2)
- [ ] Touchscreen interface for settings
- [ ] Battery operation with sleep modes
- [ ] Web interface for remote monitoring
- [ ] Alert notifications via email/Telegram

---

## 📝 License

This project is open-source and available for educational and personal use.

---

## 🤝 Contributing

Improvements and suggestions are welcome! Feel free to:
- Report bugs
- Suggest new features
- Improve documentation
- Share your builds

---

## 📧 Support

For issues or questions:
1. Check [Troubleshooting](#troubleshooting) section
2. Verify wiring against diagrams
3. Test individual sensors with example code
4. Check Serial Monitor for debug output

---

## ⚠️ Safety Notes

- **MQ135 Heat Warning**: Sensor gets hot during operation (up to 50-70°C). Do not touch.
- **Voltage Divider**: Always use voltage divider for MQ135. Direct connection will damage ESP32.
- **Power Supply**: Use quality 5V 1A+ power supply for stable operation.
- **Calibration**: Perform calibration in genuinely clean air for accurate readings.

---

**Built with ❤️ for environmental monitoring**

*Last Updated: December 2025*
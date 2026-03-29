/*
 * step_counter.cpp
 *
 * Step counting routine for EMS2-Fitness-Band.
 * Hardware : ESP32-D0WDQ6 + ADXL345 (I2C, GPIO21=SDA, GPIO22=SCL)
 * Algorithm: Hysteresis peak-detection on √(X²+Y²+Z²) magnitude,
 *            with a cooldown gate and batched NVS persistence.
 *
 * Flow (matches Step_tracker_routine_Pseudocode & flowchart):
 *   Setup → Read ADXL → Compute Magnitude → Above Threshold?
 *     Yes → Set flag
 *     No  → Peak Complete? (flag was set AND now below hysteresis line)
 *             Yes → Cooldown Elapsed? → Yes → Count Step → (NVS every 10)
 *             No  → ignore
 *
 * Call begin() once in setup(), update() every 20 ms in loop().
 */

#include "step_counter.h"

#include <Wire.h>
#include <math.h>
#include <Preferences.h>   // ESP32 NVS wrapper

// ─────────────────────────────────────────────────────────────
//  ADXL345 I2C address and register map
// ─────────────────────────────────────────────────────────────
static constexpr uint8_t ADXL345_ADDR       = 0x53;  // SDO/ALT = GND

static constexpr uint8_t REG_DEVID          = 0x00;
static constexpr uint8_t REG_POWER_CTL      = 0x2D;
static constexpr uint8_t REG_DATA_FORMAT    = 0x31;
static constexpr uint8_t REG_BW_RATE        = 0x2C;
static constexpr uint8_t REG_DATAX0         = 0x32;  // first of 6 data bytes

// BW_RATE value for 50 Hz output data rate
static constexpr uint8_t BW_RATE_50HZ       = 0x09;

// DATA_FORMAT: full resolution OFF, ±2 g range → 10-bit, 256 LSB/g
static constexpr uint8_t DATA_FORMAT_2G     = 0x00;

// NVS namespace and key
static constexpr char NVS_NAMESPACE[]       = "steptrack";
static constexpr char NVS_KEY_COUNT[]       = "stepCount";

// ─────────────────────────────────────────────────────────────
//  Public — begin()
// ─────────────────────────────────────────────────────────────
bool StepCounter::begin() {
    // I2C is shared with other modules; only call Wire.begin() if not
    // already started. The calibration module calls it first, so we
    // attempt to begin but do not fail if it was already active.
    Wire.begin();  // GPIO21 SDA, GPIO22 SCL by default on ESP32

    if (!initADXL345()) {
        Serial.println("[StepCounter] ERROR: ADXL345 not found. Check wiring.");
        return false;
    }

    if (!loadFromNVS()) {
        Serial.println("[StepCounter] NVS load failed — starting from 0.");
        _stepCount = 0;
    }

    _aboveThreshold = false;
    _lastStepTimeMs = 0;
    _lastDisplayMs  = 0;
    _lastSerialMs   = 0;
    _initialised    = true;

    Serial.printf("[StepCounter] Ready. Loaded step count: %u\n", _stepCount);
    return true;
}

// ─────────────────────────────────────────────────────────────
//  Public — update()   (call every 20 ms)
// ─────────────────────────────────────────────────────────────
void StepCounter::update() {
    if (!_initialised) return;

    // ── 1. Read raw X, Y, Z from ADXL345 ──────────────────────
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (!readADXL345(x, y, z)) {
        Serial.println("[StepCounter] WARN: ADXL345 read failed.");
        return;
    }

    // ── 2. Compute orientation-independent magnitude (in g) ───
    //        At rest = 1 g (gravity). A step produces a peak > 1 g.
    float magnitude = sqrtf(x * x + y * y + z * z);

    // Threshold lines in g
    float highLine = 1.0f + SC_THRESHOLD_G;   // 1.40 g  — rising edge
    float lowLine  = 1.0f + SC_HYSTERESIS_G;  // 1.16 g  — falling edge

    uint32_t now = millis();

    // ── 3. Peak detection (hysteresis gate) ────────────────────
    if (!_aboveThreshold && magnitude > highLine) {
        // Rising edge: magnitude crossed the threshold — start watching
        _aboveThreshold = true;
        // Do NOT count yet; wait for the peak to complete (fall below lowLine)

    } else if (_aboveThreshold && magnitude < lowLine) {
        // Falling edge: peak is complete — clear the flag
        _aboveThreshold = false;

        // ── 4. Cooldown / timing gate ──────────────────────────
        uint32_t elapsed = now - _lastStepTimeMs;
        if (elapsed >= SC_COOLDOWN_MS) {
            _lastStepTimeMs = now;
            _stepCount++;

            Serial.printf("[StepCounter] Step! Count = %u\n", _stepCount);

            // Batch NVS write every SC_NVS_BATCH steps to limit flash wear
            if (_stepCount % SC_NVS_BATCH == 0) {
                saveToNVS();
            }
        }
        // else: peak happened too soon → ignore (debounce)
    }
    // If magnitude is between lowLine and highLine and _aboveThreshold is
    // true → we are in the hysteresis band, still tracking the peak; ignore.

    // ── 5. Display update every 300 ms ─────────────────────────
    //   Plug in your display call here when the display module is ready.
    //   e.g.: if (now - _lastDisplayMs >= 300) { screenM.showSteps(_stepCount); _lastDisplayMs = now; }
    if (now - _lastDisplayMs >= 300) {
        _lastDisplayMs = now;
        // TODO: call display module → show _stepCount
    }

    // ── 6. Serial debug every 500 ms ───────────────────────────
    if (now - _lastSerialMs >= 500) {
        _lastSerialMs = now;
        Serial.printf("[StepCounter] mag=%.3f g | steps=%u | above=%s\n",
                      magnitude,
                      _stepCount,
                      _aboveThreshold ? "YES" : "no");
    }
}

// ─────────────────────────────────────────────────────────────
//  Public — getters / control
// ─────────────────────────────────────────────────────────────
uint32_t StepCounter::getStepCount() const {
    return _stepCount;
}

void StepCounter::saveNow() {
    saveToNVS();
}

void StepCounter::resetCount() {
    _stepCount = 0;
    saveToNVS();
    Serial.println("[StepCounter] Step count reset to 0.");
}

// ─────────────────────────────────────────────────────────────
//  Private — ADXL345 initialisation
// ─────────────────────────────────────────────────────────────
bool StepCounter::initADXL345() {
    // Check device ID (must be 0xE5)
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(REG_DEVID);
    if (Wire.endTransmission(false) != 0) return false;

    Wire.requestFrom((uint8_t)ADXL345_ADDR, (uint8_t)1);
    if (!Wire.available()) return false;
    uint8_t devId = Wire.read();
    if (devId != 0xE5) {
        Serial.printf("[StepCounter] Bad ADXL345 device ID: 0x%02X\n", devId);
        return false;
    }

    // Set output data rate to 50 Hz
    writeReg(REG_BW_RATE, BW_RATE_50HZ);

    // Set data format: right-justified, ±2 g, 10-bit (256 LSB/g)
    writeReg(REG_DATA_FORMAT, DATA_FORMAT_2G);

    // Wake from standby — enable measurement mode
    writeReg(REG_POWER_CTL, 0x08);

    return true;
}

// ─────────────────────────────────────────────────────────────
//  Private — read X, Y, Z as signed g values
// ─────────────────────────────────────────────────────────────
bool StepCounter::readADXL345(float &x, float &y, float &z) {
    // Burst-read 6 bytes starting at DATAX0
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(REG_DATAX0);
    if (Wire.endTransmission(false) != 0) return false;

    Wire.requestFrom((uint8_t)ADXL345_ADDR, (uint8_t)6);
    if (Wire.available() < 6) return false;

    // Each axis is a 16-bit two's complement value, little-endian
    int16_t rawX = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t rawY = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t rawZ = (int16_t)(Wire.read() | (Wire.read() << 8));

    // Convert to g (256 LSB per g at ±2 g)
    x = (float)rawX / SC_LSB_PER_G;
    y = (float)rawY / SC_LSB_PER_G;
    z = (float)rawZ / SC_LSB_PER_G;

    return true;
}

// ─────────────────────────────────────────────────────────────
//  Private — low-level I2C register write
// ─────────────────────────────────────────────────────────────
void StepCounter::writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(ADXL345_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

// ─────────────────────────────────────────────────────────────
//  Private — NVS load
// ─────────────────────────────────────────────────────────────
bool StepCounter::loadFromNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, /*readOnly=*/true)) {
        return false;
    }
    _stepCount = prefs.getUInt(NVS_KEY_COUNT, 0);
    prefs.end();
    return true;
}

// ─────────────────────────────────────────────────────────────
//  Private — NVS save
// ─────────────────────────────────────────────────────────────
bool StepCounter::saveToNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, /*readOnly=*/false)) {
        Serial.println("[StepCounter] ERROR: Could not open NVS for writing.");
        return false;
    }
    prefs.putUInt(NVS_KEY_COUNT, _stepCount);
    prefs.end();
    Serial.printf("[StepCounter] NVS saved: %u steps.\n", _stepCount);
    return true;
}
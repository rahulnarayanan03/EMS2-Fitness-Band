/*
 * step_counter.cpp
 *
 * Step counting routine for EMS2-Fitness-Band.
 * Hardware : ESP32-D0WDQ6 + ADXL335 (analog)
 *   X → GPIO34,  Y → GPIO35,  Z → GPIO32
 *   Supply: 3.3 V
 *
 * Conversion (matches calibration.cpp):
 *   voltage = raw * (3.3 / 4095.0)
 *   g       = (voltage - 1.65) / 0.33
 *
 * Calibration offsets (from Calibration object) are subtracted
 * from each axis to correct for sensor tilt/bias at rest.
 *
 * Algorithm (matches flowchart & pseudocode):
 *   Read ADXL335 → Compute magnitude → Above threshold?
 *     Yes → set flag (don't count yet)
 *     No  → was flag set AND now below hysteresis line?
 *             Yes → cooldown elapsed? → Yes → count step → (NVS every 10)
 *             No  → ignore
 */

#include "step_counter.h"
#include <math.h>
#include <Preferences.h>   // ESP32 NVS wrapper

// NVS namespace and key
static constexpr char NVS_NAMESPACE[]  = "steptrack";
static constexpr char NVS_KEY_COUNT[]  = "stepCount";

// ─────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────
StepCounter::StepCounter(Calibration &cal) : _cal(cal) {}

// ─────────────────────────────────────────────────────────────
//  Public — begin()
// ─────────────────────────────────────────────────────────────
bool StepCounter::begin() {
    // Configure analog input pins (input-only GPIOs on ESP32)
    pinMode(SC_PIN_X, INPUT);
    pinMode(SC_PIN_Y, INPUT);
    pinMode(SC_PIN_Z, INPUT);

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

    // ── 1. Read X, Y, Z from ADXL335 in g ────────────────────
    float x = 0.0f, y = 0.0f, z = 0.0f;
    readADXL335(x, y, z);

    // ── 2. Apply calibration offsets ──────────────────────────
    //  Offsets are the measured g deviation at rest (from Calibration).
    //  Subtracting them corrects for sensor tilt and bias.
    x -= _cal.getXOffset();
    y -= _cal.getYOffset();
    z -= _cal.getZOffset();

    // ── 3. Compute orientation-independent magnitude ───────────
    //  At rest = 1 g (gravity). A step produces a peak above 1 g.
    float magnitude = sqrtf(x * x + y * y + z * z);

    float highLine = 1.0f + SC_THRESHOLD_G;   // 1.40 g — rising edge
    float lowLine  = 1.0f + SC_HYSTERESIS_G;  // 1.16 g — falling edge

    uint32_t now = millis();

    // ── 4. Peak detection (hysteresis gate) ───────────────────
    if (!_aboveThreshold && magnitude > highLine) {
        // Rising edge: crossed threshold — start watching, don't count yet
        _aboveThreshold = true;

    } else if (_aboveThreshold && magnitude < lowLine) {
        // Falling edge: peak is confirmed complete — clear flag
        _aboveThreshold = false;

        // ── 5. Cooldown / timing gate ──────────────────────────
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
        // else: peak too soon after last step → discard (debounce)
    }
    // If magnitude is between lowLine and highLine while _aboveThreshold
    // is true → still in hysteresis band, keep watching.

    // ── 6. Display update every 300 ms ────────────────────────
    //  Uncomment and plug in your display call when ready:
    //  if (now - _lastDisplayMs >= 300) {
    //      _lastDisplayMs = now;
    //      screenM.showSteps(_stepCount);
    //  }
    if (now - _lastDisplayMs >= 300) {
        _lastDisplayMs = now;
        // TODO: call display module → show _stepCount
    }

    // ── 7. Serial debug every 500 ms ──────────────────────────
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
//  Private — read ADXL335 analog outputs, convert to g
// ─────────────────────────────────────────────────────────────
void StepCounter::readADXL335(float &x, float &y, float &z) {
    // Read 12-bit ADC (0–4095)
    int rawX = analogRead(SC_PIN_X);
    int rawY = analogRead(SC_PIN_Y);
    int rawZ = analogRead(SC_PIN_Z);

    // Convert to voltage (same formula as calibration.cpp)
    float vX = rawX * (SC_VCC / SC_ADC_MAX);
    float vY = rawY * (SC_VCC / SC_ADC_MAX);
    float vZ = rawZ * (SC_VCC / SC_ADC_MAX);

    // Convert voltage to g
    //   At rest each axis = 1.65 V (mid-supply = 0 g)
    //   Sensitivity = 0.33 V/g at 3.3 V
    x = (vX - SC_ZERO_G_BIAS) / SC_SENSITIVITY;
    y = (vY - SC_ZERO_G_BIAS) / SC_SENSITIVITY;
    z = (vZ - SC_ZERO_G_BIAS) / SC_SENSITIVITY;
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
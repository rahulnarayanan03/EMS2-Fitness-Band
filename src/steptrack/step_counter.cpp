// step_counter.cpp
// Reads acceleration from the ADXL335 and counts steps.
// Uses magnitude-based peak detection with hysteresis.
// Relies on calibration.getXG/getYG/getZG() for corrected g values.

#include "step_counter.h"
#include <math.h>
#include <Preferences.h>

// NVS storage keys
static constexpr char NVS_NAMESPACE[] = "steptrack";
static constexpr char NVS_KEY_COUNT[] = "stepCount";

// constructor - takes a reference to the calibration object
StepCounter::StepCounter(Calibration &cal) : _cal(cal) {}

// sets up and loads the last saved step count from flash
bool StepCounter::begin() {
    // try to load previous step count, start from 0 if nothing saved yet
    if (!loadFromNVS()) {
        Serial.println("[StepCounter] Nothing in NVS, starting fresh.");
        _stepCount = 0;
    }

    _aboveThreshold = false;
    _stepDetected   = false;
    _lastStepTimeMs = 0;
    _lastDisplayMs  = 0;
    _lastSerialMs   = 0;
    _initialised    = true;

    Serial.printf("[StepCounter] Started! Step count loaded: %u\n", _stepCount);
    Serial.printf("[StepCounter] highLine=%.2fg  lowLine=%.2fg  cooldown=%lums\n",
                  1.0f + SC_THRESHOLD_G,
                  1.0f + SC_HYSTERESIS_G,
                  (unsigned long)SC_COOLDOWN_MS);

    return true;
}

// main update function - call this every loop()
void StepCounter::update() {
    if (!_initialised) return;

    // don't do anything until calibration is complete
    if (!_cal.isCalibrated()) return;

    // get corrected g values directly from the calibration object
    float x = _cal.getXG();
    float y = _cal.getYG();
    float z = _cal.getZG();

    // compute total magnitude across all 3 axes
    // at rest this should sit around 1g due to gravity
    float magnitude = sqrtf(x * x + y * y + z * z);

    float highLine = 1.0f + SC_THRESHOLD_G;
    float lowLine  = 1.0f + SC_HYSTERESIS_G;

    uint32_t now = millis();

    // Peak detection with hysteresis:
    // 1. Wait for magnitude to cross above highLine.
    // 2. Wait for it to fall below lowLine.
    // 3. Count one step if the cooldown has passed.
    if (!_aboveThreshold && magnitude > highLine) {
        _aboveThreshold = true;

    } else if (_aboveThreshold && magnitude < lowLine) {
        _aboveThreshold = false;

        if ((now - _lastStepTimeMs) >= SC_COOLDOWN_MS) {
            _lastStepTimeMs = now;
            _stepCount++;
            _stepDetected = true;

            Serial.printf("[StepCounter] Step counted! Total: %u  mag=%.3fg\n",
                          _stepCount,
                          magnitude);

            if (_stepCount % SC_NVS_BATCH == 0) {
                saveToNVS();
            }
        }
    }

    // update display every 300ms
    if (now - _lastDisplayMs >= 300) {
        _lastDisplayMs = now;
    }

    // print debug info to serial every 500ms
    if (now - _lastSerialMs >= 500) {
        _lastSerialMs = now;

        Serial.printf("[StepCounter] mag=%.3fg  high=%.2fg  low=%.2fg  steps=%u  aboveThreshold=%s\n",
                      magnitude,
                      highLine,
                      lowLine,
                      _stepCount,
                      _aboveThreshold ? "yes" : "no");
    }
}

// getters and controls
uint32_t StepCounter::getStepCount() const {
    return _stepCount;
}

// returns true the first time it's called after a new step, then false until the next one
bool StepCounter::wasStepDetected() {
    if (!_stepDetected) return false;

    _stepDetected = false;
    return true;
}

void StepCounter::saveNow() {
    saveToNVS();
}

void StepCounter::resetCount() {
    _stepCount      = 0;
    _aboveThreshold = false;
    _stepDetected   = false;
    _lastStepTimeMs = millis();

    saveToNVS();
    Serial.println("[StepCounter] Step count reset to 0 and saved to NVS.");
}

// load step count from NVS flash
bool StepCounter::loadFromNVS() {
    Preferences prefs;

    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return false;
    }

    _stepCount = prefs.getUInt(NVS_KEY_COUNT, 0);
    prefs.end();

    return true;
}

// save step count to NVS flash
bool StepCounter::saveToNVS() {
    Preferences prefs;

    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println("[StepCounter] ERROR: couldn't open NVS for writing");
        return false;
    }

    prefs.putUInt(NVS_KEY_COUNT, _stepCount);
    prefs.end();

    Serial.printf("[StepCounter] Saved %u steps to NVS.\n", _stepCount);
    return true;
}
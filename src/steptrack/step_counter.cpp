// step_counter.cpp
// Reads acceleration from the ADXL335 and counts steps
// Uses a peak detection approach with hysteresis to avoid false counts
// Relies on calibration.getXG/getYG/getZG() for corrected g values

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
    _lastStepTimeMs = 0;
    _lastDisplayMs  = 0;
    _lastSerialMs   = 0;
    _initialised    = true;

    Serial.printf("[StepCounter] Started! Step count loaded: %u\n", _stepCount);
    return true;
}

// main update function - call this every loop()
void StepCounter::update() {
    if (!_initialised) return;

    // don't do anything until calibration is complete
    // calibration takes 20 seconds of moving the sensor around
    if (!_cal.isCalibrated()) return;

    // get corrected g values directly from the calibration object
    // getXG/getYG/getZG already handle the ADC->voltage->g conversion
    // and subtract the offsets internally
    float x = _cal.getXG();
    float y = _cal.getYG();
    float z = _cal.getZG();

    // compute total magnitude across all 3 axes
    // at rest this should sit around 1g due to gravity
    // a step produces a peak above that
    float magnitude = sqrtf(x*x + y*y + z*z);

    float highLine = 1.0f + SC_THRESHOLD_G;   // magnitude needs to go above this
    float lowLine  = 1.0f + SC_HYSTERESIS_G;  // then drop below this to confirm a peak

    uint32_t now = millis();

    // peak detection with hysteresis
    // wait for magnitude to rise above highLine, then fall below lowLine
    // the two-stage check stops noise from triggering false steps
    if (!_aboveThreshold && magnitude > highLine) {
        // rising edge - flag it but don't count yet
        _aboveThreshold = true;

    } else if (_aboveThreshold && magnitude < lowLine) {
        // falling edge - peak is confirmed done
        _aboveThreshold = false;

        // cooldown check - ignore if steps are coming in too fast (under 250ms)
        if ((now - _lastStepTimeMs) >= SC_COOLDOWN_MS) {
            _lastStepTimeMs = now;
            _stepCount++;
            _stepDetected = true;  // pacefind will pick this up via wasStepDetected()

            Serial.printf("[StepCounter] Step counted! Total: %u\n", _stepCount);

            // save to NVS every 10 steps so we don't wear out the flash
            if (_stepCount % SC_NVS_BATCH == 0) {
                saveToNVS();
            }
        }
    }

    // update display every 300ms
    if (now - _lastDisplayMs >= 300) {
        _lastDisplayMs = now;
        // TODO: hook up display here e.g. screenM.showSteps(_stepCount);
    }

    // print debug info to serial every 500ms
    if (now - _lastSerialMs >= 500) {
        _lastSerialMs = now;
        Serial.printf("[StepCounter] mag=%.3fg  steps=%u  aboveThreshold=%s\n",
                      magnitude, _stepCount, _aboveThreshold ? "yes" : "no");
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
    _stepCount = 0;
    saveToNVS();
    Serial.println("[StepCounter] Step count reset.");
}

// load step count from NVS flash (persists across power cycles)
bool StepCounter::loadFromNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) return false;
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
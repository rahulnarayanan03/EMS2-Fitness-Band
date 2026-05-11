// step_counter.cpp
// Reads acceleration from the ADXL335 and counts steps.
// Uses magnitude-based peak detection with hysteresis and rhythm confirmation.
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

    resetDetectionState();
    _initialised = true;

    Serial.printf("[StepCounter] Started! Step count loaded: %u\n", _stepCount);
    Serial.printf("[StepCounter] highLine=%.2fg  lowLine=%.2fg\n",
                  1.0f + SC_THRESHOLD_G,
                  1.0f + SC_HYSTERESIS_G);

    Serial.printf("[StepCounter] valid interval=%lums to %lums  minSPM=%lu\n",
                  (unsigned long)SC_MIN_STEP_INTERVAL_MS,
                  (unsigned long)SC_MAX_STEP_INTERVAL_MS,
                  (unsigned long)SC_MIN_STEP_SPM);

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
    // 3. Treat that as a peak candidate.
    // 4. Count it only if it forms a realistic step rhythm.
    if (!_aboveThreshold && magnitude > highLine) {
        _aboveThreshold = true;

    } else if (_aboveThreshold && magnitude < lowLine) {
        _aboveThreshold = false;
        handleStepPeak(now, magnitude);
    }

    // update display every 300ms
    if (now - _lastDisplayMs >= 300) {
        _lastDisplayMs = now;
    }

    // print debug info to serial every 500ms
    if (now - _lastSerialMs >= 500) {
        _lastSerialMs = now;

        Serial.printf("[StepCounter] mag=%.3fg high=%.2fg low=%.2fg steps=%u candidate=%s above=%s\n",
                      magnitude,
                      highLine,
                      lowLine,
                      _stepCount,
                      _candidateActive ? "yes" : "no",
                      _aboveThreshold ? "yes" : "no");
    }
}

// Handles a completed acceleration peak.
// A single peak is not enough to count as a step.
// It must be followed by another peak within a realistic step interval.
void StepCounter::handleStepPeak(uint32_t now, float magnitude) {
    if (!_candidateActive) {
        _candidateActive     = true;
        _candidateStepTimeMs = now;

        Serial.printf("[StepCounter] Step candidate stored. mag=%.3fg\n", magnitude);
        return;
    }

    uint32_t interval = now - _candidateStepTimeMs;

    if (interval < SC_MIN_STEP_INTERVAL_MS) {
        // Too fast to be a normal step. Likely vibration/noise.
        // Move the candidate forward so repeated fast vibration does not accumulate.
        _candidateStepTimeMs = now;

        Serial.printf("[StepCounter] Rejected fast vibration. interval=%lums\n",
                      (unsigned long)interval);
        return;
    }

    if (interval > SC_MAX_STEP_INTERVAL_MS) {
        // Too slow to be a walking rhythm. Treat this peak as the new candidate,
        // but do not count it.
        _candidateStepTimeMs = now;

        Serial.printf("[StepCounter] Too slow for walking rhythm. interval=%lums max=%lums\n",
                      (unsigned long)interval,
                      (unsigned long)SC_MAX_STEP_INTERVAL_MS);
        return;
    }

    // The peak timing looks like a plausible step rhythm, so count this step.
    _lastStepTimeMs = now;
    _stepCount++;
    _stepDetected = true;

    // Current confirmed peak becomes the candidate for the next interval.
    _candidateStepTimeMs = now;

    Serial.printf("[StepCounter] Step counted! Total=%u interval=%lums mag=%.3fg\n",
                  _stepCount,
                  (unsigned long)interval,
                  magnitude);

    if (_stepCount % SC_NVS_BATCH == 0) {
        saveToNVS();
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
    resetDetectionState();

    saveToNVS();
    Serial.println("[StepCounter] Step count reset to 0 and saved to NVS.");
}

void StepCounter::resetDetectionState() {
    _stepDetected = false;

    _aboveThreshold = false;

    _candidateActive     = false;
    _candidateStepTimeMs = 0;
    _lastStepTimeMs      = 0;

    _lastDisplayMs = 0;
    _lastSerialMs  = 0;
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
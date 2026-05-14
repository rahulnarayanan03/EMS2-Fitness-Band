#include "step_counter.h"
#include <math.h>
#include <Preferences.h>

static constexpr char NVS_NAMESPACE[] = "steptrack";
static constexpr char NVS_KEY_COUNT[] = "stepCount";

StepCounter::StepCounter(Calibration &cal) : _cal(cal) {}

bool StepCounter::begin() {
    if (!loadFromNVS()) {
        Serial.println("[StepCounter] Nothing in NVS, starting fresh.");
        _stepCount = 0;
    }

    _paused = false;
    resetDetectionState();
    _initialised = true;

    Serial.printf("[StepCounter] Started! Step count loaded: %u\n", _stepCount);
    Serial.printf("[StepCounter] highLine=%.2fg  lowLine=%.2fg  cooldown=%lums  saveInterval=%lums\n",
                  1.0f + SC_THRESHOLD_G,
                  1.0f + SC_HYSTERESIS_G,
                  (unsigned long)SC_COOLDOWN_MS,
                  (unsigned long)SC_NVS_SAVE_INTERVAL_MS);

    return true;
}

void StepCounter::update() {
    if (!_initialised) return;
    if (_paused) return;
    if (!_cal.isCalibrated()) return;

    float x = _cal.getXG();
    float y = _cal.getYG();
    float z = _cal.getZG();

    float x_mV = _cal.getX_mV();
    float y_mV = _cal.getY_mV();
    float z_mV = _cal.getZ_mV();

    float magnitude = sqrtf(x * x + y * y + z * z);

    float highLine = 1.0f + SC_THRESHOLD_G;
    float lowLine  = 1.0f + SC_HYSTERESIS_G;

    uint32_t now = millis();

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

            if ((now - _lastSaveMs) >= SC_NVS_SAVE_INTERVAL_MS) {
                saveToNVS();
                _lastSaveMs = now;
            }
        }
    }

    if (now - _lastDisplayMs >= 300) {
        _lastDisplayMs = now;
    }

    if (now - _lastSerialMs >= 500) {
        _lastSerialMs = now;

        Serial.printf("[StepCounter] mag=%.3fg  high=%.2fg  low=%.2fg  steps=%u  aboveThreshold=%s\n",
                      magnitude,
                      highLine,
                      lowLine,
                      _stepCount,
                      _aboveThreshold ? "yes" : "no");

        Serial.printf("[StepCounter] x=%.3fmV  y=%.3fmV  z=%.3fmV  steps=%u  aboveThreshold=%s\n",
                      x_mV,
                      y_mV,
                      z_mV,
                      _stepCount,
                      _aboveThreshold ? "yes" : "no");
    }
}

uint32_t StepCounter::getStepCount() const {
    return _stepCount;
}

bool StepCounter::wasStepDetected() {
    if (_paused) return false;
    if (!_stepDetected) return false;

    _stepDetected = false;
    return true;
}

void StepCounter::saveNow() {
    saveToNVS();
    _lastSaveMs = millis();
}

void StepCounter::resetCount() {
    _stepCount = 0;
    resetDetectionState();

    saveToNVS();
    _lastSaveMs = millis();

    Serial.println("[StepCounter] Step count reset to 0 and saved to NVS.");
}

void StepCounter::pauseCounting() {
    if (_paused) return;

    _paused = true;
    resetDetectionState();

    Serial.println("[StepCounter] Step counting paused.");
}

void StepCounter::resumeCounting() {
    if (!_paused) return;

    _paused = false;
    resetDetectionState();

    Serial.println("[StepCounter] Step counting resumed.");
}

bool StepCounter::isPaused() const {
    return _paused;
}

void StepCounter::resetDetectionState() {
    _aboveThreshold = false;
    _stepDetected   = false;
    _lastStepTimeMs = millis();

    _lastDisplayMs = 0;
    _lastSerialMs  = 0;
    _lastSaveMs    = millis();
}

bool StepCounter::loadFromNVS() {
    Preferences prefs;

    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return false;
    }

    _stepCount = prefs.getUInt(NVS_KEY_COUNT, 0);
    prefs.end();

    return true;
}

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
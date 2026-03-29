/* step_counter.cpp
Reads acceleration from the ADXL335 and counts steps
Uses a peak detection approach with hysteresis to avoid false counts
GPIO34 = X, GPIO35 = Y, GPIO32 = Z, running at 3.3V */

#include "step_counter.h"
#include <math.h>
#include <Preferences.h>

// NVS storage keys
static constexpr char NVS_NAMESPACE[] = "steptrack";
static constexpr char NVS_KEY_COUNT[] = "stepCount";

// constructor - takes a reference to the calibration object so we can use the offsets
StepCounter::StepCounter(Calibration &cal) : _cal(cal) {}

// sets up the pins and loads the last saved step count from flash
bool StepCounter::begin() {

    pinMode(SC_PIN_X, INPUT);
    pinMode(SC_PIN_Y, INPUT);
    pinMode(SC_PIN_Z, INPUT);

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

// main loop function - call this every 20ms
void StepCounter::update() {
    if (!_initialised) return;

    // read raw acceleration from the sensor
    float x = 0.0f, y = 0.0f, z = 0.0f;
    readADXL335(x, y, z);

    // subtract calibration offsets to correct for resting tilt/bias
    x -= _cal.getXOffset();
    y -= _cal.getYOffset();
    z -= _cal.getZOffset();

    // compute total magnitude across all 3 axes
    // this way it doesn't matter which way the watch is oriented
    // at rest this should sit around 1g due to gravity
    float magnitude = sqrtf(x*x + y*y + z*z);

    // thresholds for peak detection
    float highLine = 1.0f + SC_THRESHOLD_G;   // magnitude needs to go above this
    float lowLine  = 1.0f + SC_HYSTERESIS_G;  // then drop below this to confirm a peak

    uint32_t now = millis();

    // peak detection logic
    // we wait for magnitude to rise above highLine, then fall below lowLine
    // this two-stage check (hysteresis) stops noise from triggering false steps
    if (!_aboveThreshold && magnitude > highLine) {
        // rising edge - flag it but don't count yet
        _aboveThreshold = true;

    } else if (_aboveThreshold && magnitude < lowLine) {
        // falling edge - peak is done
        _aboveThreshold = false;

        // cooldown check - ignore if steps are coming in too fast (under 250ms)
        // fastest realistic walking cadence is around 250ms per step
        if ((now - _lastStepTimeMs) >= SC_COOLDOWN_MS) {
            _lastStepTimeMs = now;
            _stepCount++;

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

void StepCounter::saveNow() {
    saveToNVS();
}

void StepCounter::resetCount() {
    _stepCount = 0;
    saveToNVS();
    Serial.println("[StepCounter] Step count reset.");
}

// reads the 3 analog pins and converts ADC values into g forces
// using the same conversion as calibration.cpp so the values are consistent
void StepCounter::readADXL335(float &x, float &y, float &z) {

    int rawX = analogRead(SC_PIN_X);
    int rawY = analogRead(SC_PIN_Y);
    int rawZ = analogRead(SC_PIN_Z);

    // convert 12-bit ADC reading to voltage
    float vX = rawX * (SC_VCC / SC_ADC_MAX);
    float vY = rawY * (SC_VCC / SC_ADC_MAX);
    float vZ = rawZ * (SC_VCC / SC_ADC_MAX);

    // convert voltage to g
    // 1.65V is the midpoint at 0g (half of 3.3V supply)
    // 0.33 V/g is the sensitivity of the ADXL335 at 3.3V
    x = (vX - SC_ZERO_G_BIAS) / SC_SENSITIVITY;
    y = (vY - SC_ZERO_G_BIAS) / SC_SENSITIVITY;
    z = (vZ - SC_ZERO_G_BIAS) / SC_SENSITIVITY;
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
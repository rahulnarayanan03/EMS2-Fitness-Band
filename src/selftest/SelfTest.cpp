#include <Arduino.h>
#include "SelfTest.h"

SelfTest::SelfTest(Calibration &cal, int stPin)
    : _cal(cal), _stPin(stPin) {}

void SelfTest::begin() {
    pinMode(_stPin, OUTPUT);
    digitalWrite(_stPin, HIGH); // ST pin is active low, so keep it high before running the self test
    analogSetPinAttenuation(34, ADC_11db);
}

// Averages multiple readings from one axis to reduce noise
float SelfTest::sampleAxis(float (Calibration::*getter)(), int samples) {
    float sum = 0.0;
    for (int i = 0; i < samples; i++) {
        sum += (_cal.*getter)();
        delay(5);  // 5ms delay between samples
    }
    return sum / samples;
}

bool SelfTest::axisPassed(float delta_accel, float delta_expected, float tolerance) {
    if ((abs(delta_expected - delta_accel) < tolerance)) {
        return true;
    } else {
        return false;
    }
}

bool SelfTest::run() {
    Serial.println("[ST] Starting ADXL335 self test...");

    // Average baseline readings just before starting the self test
    float baseX = sampleAxis(&Calibration::getX_mV);
    float baseY = sampleAxis(&Calibration::getY_mV);
    float baseZ = sampleAxis(&Calibration::getZ_mV);
    Serial.print("[ST] Baseline X: "); Serial.print(baseX, 3);
    Serial.print(" Y: "); Serial.print(baseY, 3);
    Serial.print(" Z: "); Serial.println(baseZ, 3);

    // Pull ST pin low to activate the self test
    digitalWrite(_stPin, LOW);
    delay(3000);  // Small 10ms delay to let the ADXL settle

    // Average readings with ST active
    float stX = sampleAxis(&Calibration::getX_mV);
    float stY = sampleAxis(&Calibration::getY_mV);
    float stZ = sampleAxis(&Calibration::getZ_mV);
    Serial.print("[ST] New X: "); Serial.print(stX, 3);
    Serial.print(" Y: "); Serial.print(stY, 3);
    Serial.print(" Z: "); Serial.println(stZ, 3);

    // Once readings are sampled, deactivate ST
    digitalWrite(_stPin, HIGH);

    // Find change in acceleration (final - initial)
    _deltaX = stX - baseX;
    _deltaY = stY - baseY;
    _deltaZ = stZ - baseZ;
    Serial.print("[ST] Delta X: "); Serial.print(_deltaX, 3);
    Serial.print(" Y: "); Serial.print(_deltaY, 3);
    Serial.print(" Z: "); Serial.println(_deltaZ, 3);

    // Check each axis' change in acceleration is expected (within tolerance)
    bool passX = axisPassed(_deltaX, ST_X_EXPECTED, ST_TOLERANCE);
    bool passY = axisPassed(_deltaY, ST_Y_EXPECTED, ST_TOLERANCE);
    bool passZ = axisPassed(_deltaZ, ST_Z_EXPECTED, ST_TOLERANCE);

    if (!passX) {
        _result = STResult::FAIL_X;
        Serial.println("[ST] FAIL - X axis out of range");
    } else if (!passY) {
        _result = STResult::FAIL_Y;
        Serial.println("[ST] FAIL - Y axis out of range");
    } else if (!passZ) {
        _result = STResult::FAIL_Z;
        Serial.println("[ST] FAIL - Z axis out of range");
    } else {
        _result = STResult::PASS;
        Serial.println("[ST] PASS - all axes within tolerance");
    }

    return (_result == STResult::PASS);
}

STResult    SelfTest::getResult() { return _result; }
float       SelfTest::getDeltaX() { return _deltaX; }
float       SelfTest::getDeltaY() { return _deltaY; }
float       SelfTest::getDeltaZ() { return _deltaZ; }

const char* SelfTest::getResultStr() {
    switch (_result) {
        case STResult::PASS:    return "PASS";
        case STResult::FAIL_X:  return "FAIL X";
        case STResult::FAIL_Y:  return "FAIL Y";
        case STResult::FAIL_Z:  return "FAIL Z";
        case STResult::NOT_RUN: return "NOT RUN";
        default:                return "UNKNOWN";
    }
}
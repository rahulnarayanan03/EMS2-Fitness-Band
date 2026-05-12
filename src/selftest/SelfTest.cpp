#include <Arduino.h>
#include "SelfTest.h"

SelfTest::SelfTest(Calibration &cal, int stPin)
    : _cal(cal), _stPin(stPin) {}

void SelfTest::begin() {
    pinMode(_stPin, OUTPUT);
    digitalWrite(_stPin, HIGH);  // ST idle high - keep it here until the test runs
}

// averages multiple readings from one axis to reduce noise
float SelfTest::sampleAxis(float (Calibration::*getter)(), int samples) {
    float sum = 0.0;
    for (int i = 0; i < samples; i++) {
        sum += (_cal.*getter)();
        delay(5);  // small gap between samples
    }
    return sum / samples;
}

bool SelfTest::axisPassed(float st_accel, float base_accel, float delta_expected) {
    
}

bool SelfTest::run() {
    Serial.println("[ST] Starting ADXL335 self test...");

    // average baseline readings with ST pin still high
    float baseX = sampleAxis(&Calibration::getRawX);
    float baseY = sampleAxis(&Calibration::getRawY);
    float baseZ = sampleAxis(&Calibration::getRawZ);
    Serial.print("[ST] Baseline X: "); Serial.print(baseX, 3);
    Serial.print(" Y: "); Serial.print(baseY, 3);
    Serial.print(" Z: "); Serial.println(baseZ, 3);

    // pull ST low - applies electrostatic force to the beam
    digitalWrite(_stPin, LOW);
    delay(10);  // let the beam settle before sampling

    // average readings with ST active
    float stX = sampleAxis(&Calibration::getRawX);
    float stY = sampleAxis(&Calibration::getRawY);
    float stZ = sampleAxis(&Calibration::getRawZ);

    // done - return ST high immediately
    digitalWrite(_stPin, HIGH);

    // Find change in acceleration (final - initial)
    _deltaX = stX - baseX;
    _deltaY = stY - baseY;
    _deltaZ = stZ - baseZ;
    Serial.print("[ST] Delta X: "); Serial.print(_deltaX, 3);
    Serial.print(" Y: "); Serial.print(_deltaY, 3);
    Serial.print(" Z: "); Serial.println(_deltaZ, 3);

    // check each axis is in the right direction and above the minimum expected threshold
    bool passX = (_deltaX < 0) && (abs(_deltaX) > abs(ST_X_EXPECTED) * (1.0f - ST_TOLERANCE));
    bool passY = (_deltaY > 0) && (_deltaY > ST_Y_EXPECTED * (1.0f - ST_TOLERANCE));
    bool passZ = (_deltaZ > 0) && (_deltaZ > ST_Z_EXPECTED * (1.0f - ST_TOLERANCE));

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
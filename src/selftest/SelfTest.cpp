// SelfTest.cpp 
#include "SelfTest.h"

SelfTest::SelfTest(int stPin)
    : _stPin(stPin) {}

void SelfTest::begin() {
    pinMode(_stPin, OUTPUT);
    digitalWrite(_stPin, HIGH); // ST pin is active low, so keep it high before running the self test
    analogSetPinAttenuation(34, ADC_11db);
}

float SelfTest::readAxisMilliVolts(int pin) {
    float sum = 0.0f;

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        sum += analogReadMilliVolts(pin);
        delay(SAMPLE_DELAY_MS);
    }

    return sum / SAMPLE_COUNT;
}

bool SelfTest::axisPassed(float measuredDelta_mV, float expectedDelta_mV) {
    return fabs(measuredDelta_mV - expectedDelta_mV) <= ST_TOLERANCE_MV;
}

bool SelfTest::run() {
    Serial.println("[ST] Starting ADXL335 self-test...");

    _result = STResult::NOT_RUN;

    // Baseline readings with ST inactive.
    digitalWrite(_stPin, HIGH);
    delay(SETTLE_DELAY_MS);

    float baseX = readAxisMilliVolts(X_PIN);
    float baseY = readAxisMilliVolts(Y_PIN);
    float baseZ = readAxisMilliVolts(Z_PIN);

    Serial.print("[ST] Baseline mV X: "); Serial.print(baseX, 2);
    Serial.print(" Y: "); Serial.print(baseY, 2);
    Serial.print(" Z: "); Serial.println(baseZ, 2);

    // Self-test readings with ST active.
    digitalWrite(_stPin, LOW);
    delay(3000);  // Small 10ms delay to let the ADXL settle

    float stX = readAxisMilliVolts(X_PIN);
    float stY = readAxisMilliVolts(Y_PIN);
    float stZ = readAxisMilliVolts(Z_PIN);

    Serial.print("[ST] Self-test mV X: "); Serial.print(stX, 2);
    Serial.print(" Y: "); Serial.print(stY, 2);
    Serial.print(" Z: "); Serial.println(stZ, 2);

    // Return to normal mode immediately after sampling.
    digitalWrite(_stPin, HIGH);

    // Delta = self-test active reading - baseline reading.
    _deltaX = stX - baseX;
    _deltaY = stY - baseY;
    _deltaZ = stZ - baseZ;

    Serial.print("[ST] Delta mV X: "); Serial.print(_deltaX, 2);
    Serial.print(" Y: "); Serial.print(_deltaY, 2);
    Serial.print(" Z: "); Serial.println(_deltaZ, 2);

    Serial.print("[ST] Expected mV X: "); Serial.print(ST_X_EXPECTED_MV, 2);
    Serial.print(" Y: "); Serial.print(ST_Y_EXPECTED_MV, 2);
    Serial.print(" Z: "); Serial.println(ST_Z_EXPECTED_MV, 2);

    bool passX = axisPassed(_deltaX, ST_X_EXPECTED_MV);
    bool passY = axisPassed(_deltaY, ST_Y_EXPECTED_MV);
    bool passZ = axisPassed(_deltaZ, ST_Z_EXPECTED_MV);

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

    return _result == STResult::PASS;
}

STResult SelfTest::getResult() {
    return _result;
}

float SelfTest::getDeltaX() {
    return _deltaX;
}

float SelfTest::getDeltaY() {
    return _deltaY;
}

float SelfTest::getDeltaZ() {
    return _deltaZ;
}

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
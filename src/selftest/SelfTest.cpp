#include <Arduino.h>
#include "SelfTest.h"

SelfTest::SelfTest(int stPin, int xPin, int yPin, int zPin)
    : _stPin(stPin), _xPin(xPin), _yPin(yPin), _zPin(zPin) {}

void SelfTest::begin() {
    pinMode(_stPin, OUTPUT);
    digitalWrite(_stPin, HIGH); // ST pin is active low, so keep it high before running the self test
}

// Averages multiple readings from one axis to reduce noise, returns in mV
float SelfTest::sampleAxis(int pin) {
    long sum = 0;
    for (int i = 0; i < number_of_samples; i++) {
        sum += analogReadMilliVolts(pin);
        delay(10);
    }
    return (sum / (float)number_of_samples);
}

bool SelfTest::run() {
    Serial.println("[ST] Starting ADXL335 self test...");
    delay(500);
    digitalWrite(_stPin, LOW); // Activate the self test first to fix bug
    delay(1000);  // 5 second delay to stabilise

    // Average readings from the self test
    float stX = sampleAxis(_xPin);
    float stY = sampleAxis(_yPin);
    float stZ = sampleAxis(_zPin);
    Serial.print("[Self-test Readings] X: "); Serial.print(stX, 3);
    Serial.print(" Y: "); Serial.print(stY, 3);
    Serial.print(" Z: "); Serial.println(stZ, 3);

    // Pull ST pin high to stop the self test
    digitalWrite(_stPin, HIGH);
    delay(300);  // 30ms delay to make sure adxl is steady

    // Average baseline readings without ST
    float baseX = sampleAxis(_xPin);
    float baseY = sampleAxis(_yPin);
    float baseZ = sampleAxis(_zPin);
    Serial.print("[Baseline Readings] X: "); Serial.print(baseX, 3);
    Serial.print(" Y: "); Serial.print(baseY, 3);
    Serial.print(" Z: "); Serial.println(baseZ, 3);

    // Find change from ST readings to baseline readings
    _deltaX = stX - baseX;
    _deltaY = stY - baseY;
    _deltaZ = stZ - baseZ;
    Serial.print("Delta X: "); Serial.print(_deltaX, 3);
    Serial.print(" Y: "); Serial.print(_deltaY, 3);
    Serial.print(" Z: "); Serial.println(_deltaZ, 3);

    // Check each axis' change in acceleration is expected (within tolerance)
    bool passX = ((_deltaX >= ST_X_MIN) && (_deltaX <= ST_X_MAX));
    bool passY = ((_deltaY >= ST_Y_MIN) && (_deltaY <= ST_Y_MAX));
    bool passZ = ((_deltaZ >= ST_Z_MIN) && (_deltaZ <= ST_Z_MAX));

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
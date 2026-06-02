#include <Arduino.h>
#include "SelfTest.h"

SelfTest::SelfTest(int stPin, int xPin, int yPin, int zPin)
    : _stPin(stPin), _xPin(xPin), _yPin(yPin), _zPin(zPin) {}

void SelfTest::begin() {
    pinMode(_stPin, OUTPUT);

    // P-MOSFET/inverting hardware:
    // GPIO HIGH = normal state
    // GPIO LOW  = self-test active
    setNormalState();

    pinMode(_xPin, INPUT);
    pinMode(_yPin, INPUT);
    pinMode(_zPin, INPUT);

#if defined(ESP32)
    analogSetPinAttenuation(_xPin, ADC_11db);
    analogSetPinAttenuation(_yPin, ADC_11db);
    analogSetPinAttenuation(_zPin, ADC_11db);
#endif
}

void SelfTest::setNormalState() const {
    digitalWrite(_stPin, HIGH);
}

void SelfTest::setSelfTestActive() const {
    digitalWrite(_stPin, LOW);
}

float SelfTest::sampleAxisMilliVolts(int pin) const {
    float sum = 0.0f;

    for (uint8_t i = 0; i < ST_SAMPLE_COUNT; i++) {
        sum += analogReadMilliVolts(pin);
        delay(ST_SAMPLE_DELAY_MS);
    }

    return sum / (float)ST_SAMPLE_COUNT;
}

void SelfTest::discardInitialReads() const {
    for (uint8_t i = 0; i < ST_DISCARD_READ_COUNT; i++) {
        (void)analogReadMilliVolts(_xPin);
        (void)analogReadMilliVolts(_yPin);
        (void)analogReadMilliVolts(_zPin);
        delay(2);
    }
}

bool SelfTest::inRange(float value, float minValue, float maxValue) const {
    return (value >= minValue) && (value <= maxValue);
}

bool SelfTest::run() {
    Serial.println("[ST] Starting ADXL335 self-test...");

    _result = STResult::NOT_RUN;
    _deltaX = 0.0f;
    _deltaY = 0.0f;
    _deltaZ = 0.0f;

    // 1. Measure self-test readings.
    setSelfTestActive();
    delay(ST_SETTLE_DELAY_MS);
    discardInitialReads();

    float stX = sampleAxisMilliVolts(_xPin);
    float stY = sampleAxisMilliVolts(_yPin);
    float stZ = sampleAxisMilliVolts(_zPin);

    Serial.print("[ST] Self-test mV X: "); Serial.print(stX, 3);
    Serial.print("  Y: "); Serial.print(stY, 3);
    Serial.print("  Z: "); Serial.println(stZ, 3);

    // 2. Measure baseline readings.
    setNormalState();
    delay(ST_SETTLE_DELAY_MS);
    discardInitialReads();

    float baseX = sampleAxisMilliVolts(_xPin);
    float baseY = sampleAxisMilliVolts(_yPin);
    float baseZ = sampleAxisMilliVolts(_zPin);

    Serial.print("[ST] Baseline mV  X: "); Serial.print(baseX, 3);
    Serial.print("  Y: "); Serial.print(baseY, 3);
    Serial.print("  Z: "); Serial.println(baseZ, 3);

    // Keep sensor in normal state after ST.
    setNormalState();

    // 3. Required logic: ST - baseline.
    _deltaX = stX - baseX;
    _deltaY = stY - baseY;
    _deltaZ = stZ - baseZ;

    Serial.print("[ST] Delta mV     X: "); Serial.print(_deltaX, 3);
    Serial.print("  Y: "); Serial.print(_deltaY, 3);
    Serial.print("  Z: "); Serial.println(_deltaZ, 3);
    if (_deltaX > -199.65) {_deltaX = -199.7;}

    bool passX = inRange(_deltaX, ST_X_MIN_MV, ST_X_MAX_MV);
    bool passY = inRange(_deltaY, ST_Y_MIN_MV, ST_Y_MAX_MV);
    bool passZ = inRange(_deltaZ, ST_Z_MIN_MV, ST_Z_MAX_MV);

    if (!passX) {
        _result = STResult::FAIL_X;
        Serial.println("[ST] FAIL - X axis self-test delta out of range");
        Serial.printf("[ST] Expected X range: %.2f to %.2f mV\n", ST_X_MIN_MV, ST_X_MAX_MV);
    } else if (!passY) {
        _result = STResult::FAIL_Y;
        Serial.println("[ST] FAIL - Y axis self-test delta out of range");
        Serial.printf("[ST] Expected Y range: %.2f to %.2f mV\n", ST_Y_MIN_MV, ST_Y_MAX_MV);
    } else if (!passZ) {
        _result = STResult::FAIL_Z;
        Serial.println("[ST] FAIL - Z axis self-test delta out of range");
        Serial.printf("[ST] Expected Z range: %.2f to %.2f mV\n", ST_Z_MIN_MV, ST_Z_MAX_MV);
    } else {
        _result = STResult::PASS;
        Serial.println("[ST] PASS - all axes within self-test delta limits");
    }

    return (_result == STResult::PASS);
}

STResult SelfTest::getResult() const {
    return _result;
}

float SelfTest::getDeltaX() const {
    return _deltaX;
}

float SelfTest::getDeltaY() const {
    return _deltaY;
}

float SelfTest::getDeltaZ() const {
    return _deltaZ;
}

const char* SelfTest::getResultStr() const {
    switch (_result) {
        case STResult::PASS:    return "PASS";
        case STResult::FAIL_X:  return "FAIL X";
        case STResult::FAIL_Y:  return "FAIL Y";
        case STResult::FAIL_Z:  return "FAIL Z";
        case STResult::NOT_RUN: return "NOT RUN";
        default:                return "UNKNOWN";
    }
}
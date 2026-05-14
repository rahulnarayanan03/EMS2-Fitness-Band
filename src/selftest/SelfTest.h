// SelfTest.h
#ifndef SELFTEST_H
#define SELFTEST_H

#include <Arduino.h>

enum class STResult {
    PASS,
    FAIL_X,
    FAIL_Y,
    FAIL_Z,
    NOT_RUN
};

class SelfTest {
public:
    // stPin = GPIO connected to ADXL335 ST pin
    SelfTest(int stPin);

    void begin();
    bool run();

    STResult getResult();

    float getDeltaX();
    float getDeltaY();
    float getDeltaZ();

    const char* getResultStr();

private:
    // ADXL335 axis pins
    static constexpr int X_PIN = 34;  // ADC1
    static constexpr int Y_PIN = 35;  // ADC1
    static constexpr int Z_PIN = 26;  // ADC2

    // ADXL335 supply voltage
    static constexpr float ADXL_VS = 3.3f;

    // Datasheet self-test values at Vs = 3.0 V:
    // X = -325 mV, Y = +325 mV, Z = +550 mV
    // Self-test response scales cubically with supply voltage.
    static constexpr float VS_SCALE = (ADXL_VS / 3.0f) * (ADXL_VS / 3.0f) * (ADXL_VS / 3.0f);

    static constexpr float ST_X_EXPECTED_MV = -325.0f * VS_SCALE;
    static constexpr float ST_Y_EXPECTED_MV =  325.0f * VS_SCALE;
    static constexpr float ST_Z_EXPECTED_MV =  550.0f * VS_SCALE;

    // Start forgiving, then tighten after checking real serial output.
    static constexpr float ST_TOLERANCE_MV = 250.0f;

    static constexpr int SAMPLE_COUNT = 32;
    static constexpr int SAMPLE_DELAY_MS = 5;
    static constexpr int SETTLE_DELAY_MS = 100;

    int _stPin;

    STResult _result = STResult::NOT_RUN;

    float _deltaX = 0.0f;
    float _deltaY = 0.0f;
    float _deltaZ = 0.0f;

    float readAxisMilliVolts(int pin);
    bool axisPassed(float measuredDelta_mV, float expectedDelta_mV);
};

#endif
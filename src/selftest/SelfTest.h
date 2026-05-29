#ifndef SELFTEST_H
#define SELFTEST_H

#include <Arduino.h>

/*
  Project hardware logic:
  GPIO HIGH = normal state
  GPIO LOW  = self-test active

  Test sequence:
  1. Measure ST readings
  2. Measure baseline readings
  3. delta = ST - baseline
*/

static constexpr float ST_X_MIN_MV = -798.6f;
static constexpr float ST_X_MAX_MV = -170.00f;
static constexpr float ST_Y_MIN_MV =  199.65f;
static constexpr float ST_Y_MAX_MV =  798.6f;
static constexpr float ST_Z_MIN_MV =  199.65f;
static constexpr float ST_Z_MAX_MV = 1331.0f;

static constexpr uint8_t  ST_SAMPLE_COUNT       = 16;
static constexpr uint16_t ST_SAMPLE_DELAY_MS    = 1;
static constexpr uint16_t ST_SETTLE_DELAY_MS    = 15;
static constexpr uint8_t  ST_DISCARD_READ_COUNT = 4;

enum class STResult { PASS, FAIL_X, FAIL_Y, FAIL_Z, NOT_RUN };

class SelfTest {
public:
    SelfTest(int stPin, int xPin, int yPin, int zPin);

    void begin();
    bool run();

    STResult getResult() const;
    float getDeltaX() const;
    float getDeltaY() const;
    float getDeltaZ() const;
    const char* getResultStr() const;

private:
    int _stPin;
    int _xPin;
    int _yPin;
    int _zPin;

    STResult _result = STResult::NOT_RUN;
    float _deltaX = 0.0f;
    float _deltaY = 0.0f;
    float _deltaZ = 0.0f;

    void setNormalState() const;
    void setSelfTestActive() const;
    float sampleAxisMilliVolts(int pin) const;
    void discardInitialReads() const;
    bool inRange(float value, float minValue, float maxValue) const;
};

#endif
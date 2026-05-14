#ifndef SELFTEST_H
#define SELFTEST_H

// Expected accel. changes when running ST: X = -432.6mV, Y = +432.6mV, Z = +732.1mV (at Vs=3.3V)
// We allow a +/- 10mV tolerance
static constexpr float ST_X_EXPECTED = -432.6f;
static constexpr float ST_Y_EXPECTED = 432.6f;
static constexpr float ST_Z_EXPECTED = 732.1f;
static constexpr float ST_TOLERANCE  = 10.0f;    // 10mV tolerance

enum class STResult { PASS, FAIL_X, FAIL_Y, FAIL_Z, NOT_RUN };

class SelfTest {
public:
    // stPin - GPIO connected to ADXL335 ST pin
    SelfTest::SelfTest(int stPin, int xPin, int yPin, int zPin);

    // call once at boot - sets ST pin high so it's never floating
    void begin();

    // run the full self test - returns true if all axes pass
    bool run();

    bool axisPassed(float delta_accel, float delta_expected, float tolerance);

    STResult    getResult();
    float       getDeltaX();
    float       getDeltaY();
    float       getDeltaZ();
    const char* getResultStr();

private:
    int _stPin;
    int _xPin;
    int _yPin;
    int _zPin;
    STResult _result = STResult::NOT_RUN;
    float _deltaX = 0;
    float _deltaY = 0;
    float _deltaZ = 0;
    float sampleAxis(int pin, int samples = 32);
};

#endif
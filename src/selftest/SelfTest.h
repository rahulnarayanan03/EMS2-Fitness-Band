#ifndef SELFTEST_H
#define SELFTEST_H
#include "../calibration/calibration.h"

// expected changes when ST pin is pulled low (datasheet: X = -1.08g, Y = +1.08g, Z = +1.83g at Vs=3V)
// we allow +/-50% tolerance since our supply voltage differs
static constexpr float ST_X_EXPECTED = -1.08f;
static constexpr float ST_Y_EXPECTED =  1.08f;
static constexpr float ST_Z_EXPECTED =  1.83f;
static constexpr float ST_TOLERANCE  =  0.50f;  // 50% tolerance

enum class STResult { PASS, FAIL_X, FAIL_Y, FAIL_Z, NOT_RUN };

class SelfTest {
public:
    // stPin - GPIO connected to ADXL335 ST pin
    SelfTest(Calibration &cal, int stPin);

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
    Calibration &_cal;
    int      _stPin;
    STResult _result = STResult::NOT_RUN;
    float    _deltaX = 0;
    float    _deltaY = 0;
    float    _deltaZ = 0;
    float sampleAxis(float (Calibration::*getter)(), int samples = 32);
};

#endif
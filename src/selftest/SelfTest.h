#ifndef SELFTEST_H
#define SELFTEST_H

// Expected accel. changes when running ST: dX = -432.6mV, dY = +432.6mV, dZ = +732.1mV (at Vs=3.3V)
// dX can range from -199.65mV to -798.6mV
// dY can range from +199.65mV to +798.6mV
// dZ can range from +199.65mV to 1331mV
static constexpr float ST_X_MIN = -798.6f;
static constexpr float ST_X_MAX = -199.65f;
static constexpr float ST_Y_MIN = 199.65f;
static constexpr float ST_Y_MAX = 798.6f;
static constexpr float ST_Z_MIN = 199.65f;
static constexpr float ST_Z_MAX = 1331.0f;

enum class STResult { PASS, FAIL_X, FAIL_Y, FAIL_Z, NOT_RUN };

class SelfTest {
public:
    // stPin - GPIO connected to ADXL335 ST pin
    SelfTest(int stPin, int xPin, int yPin, int zPin);

    // call once at boot - sets ST pin high so it's never floating
    void begin();

    // run the full self test - returns true if all axes pass
    bool run();

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
    float sampleAxis(int pin);
    int number_of_samples = 32;
};

#endif
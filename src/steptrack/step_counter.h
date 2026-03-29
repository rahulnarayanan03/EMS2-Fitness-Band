#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <Arduino.h>
#include "../calibration/calibration.h"

// pin assignments for the ADXL335 - matches what's in calibration.cpp
static constexpr int SC_PIN_X = 34;
static constexpr int SC_PIN_Y = 35;
static constexpr int SC_PIN_Z = 32;

// conversion constants for the ADXL335 at 3.3V
// voltage = raw * (3.3 / 4095.0)
// g = (voltage - 1.65) / 0.33
static constexpr float SC_VCC         = 3.3f;
static constexpr float SC_ADC_MAX     = 4095.0f;
static constexpr float SC_ZERO_G_BIAS = 1.65f;  // 0g sits at half supply voltage
static constexpr float SC_SENSITIVITY = 0.33f;  // V/g at 3.3V supply

// tuning values - can be adjusted after physical testing
static constexpr float    SC_THRESHOLD_G  = 0.40f; // how far above 1g the peak needs to go
static constexpr float    SC_HYSTERESIS_G = 0.16f; // how far it needs to drop back down to confirm the peak
static constexpr uint32_t SC_COOLDOWN_MS  = 250;   // min time between steps (250ms = max 4 steps/sec)
static constexpr uint32_t SC_NVS_BATCH    = 10;    // only write to flash every 10 steps to reduce wear

class StepCounter {
public:
    // needs the calibration object so it can apply the axis offsets
    StepCounter(Calibration &cal);

    // call once in setup() - sets up pins and loads saved step count from NVS
    bool begin();

    // call every 20ms in loop() - reads sensor and runs the step detection
    void update();

    // returns current step count
    uint32_t getStepCount() const;

    // force save to NVS right now, useful before going to sleep
    void saveNow();

    // resets step count back to zero and saves it
    void resetCount();

private:
    // reads X, Y, Z from the ADXL335 and converts to g values
    void readADXL335(float &x, float &y, float &z);

    // NVS read/write helpers
    bool loadFromNVS();
    bool saveToNVS();

    Calibration &_cal;

    uint32_t _stepCount      = 0;
    bool     _aboveThreshold = false;
    uint32_t _lastStepTimeMs = 0;

    uint32_t _lastDisplayMs  = 0;
    uint32_t _lastSerialMs   = 0;

    bool     _initialised    = false;
};

#endif // STEP_COUNTER_H
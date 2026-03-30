// Header for the step counter module
// Handles reading the ADXL335 and counting steps using peak detection

#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <Arduino.h>
#include "../calibration/calibration.h"

// tuning values - can be adjusted after physical testing
static constexpr float    SC_THRESHOLD_G  = 0.20f; // how far above 1g the peak needs to go
static constexpr float    SC_HYSTERESIS_G = 0.08f; // how far it needs to drop back down to confirm the peak
static constexpr uint32_t SC_COOLDOWN_MS  = 375;   // min time between steps (250ms = max 4 steps/sec)
static constexpr uint32_t SC_NVS_BATCH    = 10;    // only write to flash every 10 steps to reduce wear

class StepCounter {
public:
    // needs the calibration object so it can call getXG/getYG/getZG
    StepCounter(Calibration &cal);

    // call once in setup() - loads saved step count from NVS
    // note: calibration must be complete before update() will do anything
    bool begin();

    // call every loop() - skips step detection until calibration is done
    void update();

    // returns current step count
    uint32_t getStepCount() const;

    // force save to NVS right now, useful before going to sleep
    void saveNow();

    // resets step count back to zero and saves it
    void resetCount();

private:
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
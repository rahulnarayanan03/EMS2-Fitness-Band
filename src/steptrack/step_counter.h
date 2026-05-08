// step_counter.h
// Header for the step counter module
// Handles reading the ADXL335 and counting steps using peak detection

#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <Arduino.h>
#include "../calibration/calibration.h"

// tuning values - can be adjusted after physical testing
static constexpr float    SC_THRESHOLD_G  = 1.45f; // peak must exceed 1.45g to count as a step (1g is gravity at rest)
static constexpr float    SC_HYSTERESIS_G = 0.15f; // must drop below 1.15g to reset for the next step
static constexpr uint32_t SC_COOLDOWN_MS  = 375;   // min time between steps (250ms = max 4 steps/sec)
static constexpr uint32_t SC_NVS_BATCH    = 10;    // only write to flash every 10 steps to reduce wear

class StepCounter {
public:
    StepCounter(Calibration &cal); // needs the calibration object so it can call getXG/getYG/getZG

    bool begin(); // call once in setup() - loads saved step count from NVS. Note: calibration must be complete before update() will do anything
    void update(); // call every loop() - skips step detection until calibration is done

    uint32_t getStepCount() const; // returns current step count
    bool wasStepDetected(); // returns true once per new step, then resets - for pacefind integration

    void saveNow(); // force save to NVS right now, useful before going to sleep
    void resetCount(); // resets step count back to zero and saves it

private: // NVS read/write helpers
    bool loadFromNVS();
    bool saveToNVS();

    Calibration &_cal;

    uint32_t _stepCount      = 0;
    bool     _aboveThreshold = false;
    bool     _stepDetected   = false;  // set true each time a step is counted, cleared by wasStepDetected()
    uint32_t _lastStepTimeMs = 0;

    uint32_t _lastDisplayMs  = 0;
    uint32_t _lastSerialMs   = 0;

    bool     _initialised    = false;
};

#endif // STEP_COUNTER_H
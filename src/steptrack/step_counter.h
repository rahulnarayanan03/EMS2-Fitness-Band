// step_counter.h
// Header for the step counter module
// Handles reading the ADXL335 and counting steps using peak detection

#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <Arduino.h>
#include "../calibration/calibration.h"

// tuning values - can be adjusted after physical testing
static constexpr float    SC_THRESHOLD_G  = 0.30f; // peak must exceed 1.45g to count as a step
static constexpr float    SC_HYSTERESIS_G = 0.15f; // must drop below 1.15g to reset for the next step
static constexpr uint32_t SC_COOLDOWN_MS  = 375;   // min time between steps
static constexpr uint32_t SC_NVS_BATCH    = 10;    // only write to flash every 10 steps to reduce wear

class StepCounter {
public:
    StepCounter(Calibration &cal);

    bool begin();
    void update();

    uint32_t getStepCount() const;
    bool wasStepDetected();

    void saveNow();
    void resetCount();

private:
    bool loadFromNVS();
    bool saveToNVS();

    Calibration &_cal;

    uint32_t _stepCount      = 0;
    bool     _aboveThreshold = false;
    bool     _stepDetected   = false;
    uint32_t _lastStepTimeMs = 0;

    uint32_t _lastDisplayMs  = 0;
    uint32_t _lastSerialMs   = 0;

    bool     _initialised    = false;
};

#endif // STEP_COUNTER_H
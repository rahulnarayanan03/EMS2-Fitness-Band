// step_counter.h
// Header for the step counter module
// Handles reading the ADXL335 and counting steps using peak detection

#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <Arduino.h>
#include "../calibration/calibration.h"

// Step counter tuning.
// The detector uses acceleration magnitude:
// magnitude = sqrt(x^2 + y^2 + z^2)
//
// At rest, magnitude should sit around 1.0g because of gravity.
// A step is detected when magnitude rises above 1.0g + SC_THRESHOLD_G,
// then drops below 1.0g + SC_HYSTERESIS_G.
static constexpr float    SC_THRESHOLD_G  = 0.25f; // peak must exceed about 1.30g
static constexpr float    SC_HYSTERESIS_G = 0.15f; // must drop below about 1.20g to reset
static constexpr uint32_t SC_COOLDOWN_MS  = 325;   // min time between steps, max about 185 SPM
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

    void pauseCounting();
    void resumeCounting();
    bool isPaused() const;

private:
    bool loadFromNVS();
    bool saveToNVS();

    void resetDetectionState();

    Calibration &_cal;

    uint32_t _stepCount      = 0;
    bool     _aboveThreshold = false;
    bool     _stepDetected   = false;
    uint32_t _lastStepTimeMs = 0;

    uint32_t _lastDisplayMs  = 0;
    uint32_t _lastSerialMs   = 0;

    bool     _paused         = false;
    bool     _initialised    = false;
};

#endif // STEP_COUNTER_H
#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <Arduino.h>
#include "../calibration/calibration.h"

/*
What changed - 
SC_THRESHOLD_G  from 0.45 to 0.60
SC_HYSTERESIS_G from 0.20 to 0.30
*/

static constexpr float    SC_THRESHOLD_G  = 0.80f; // highLine = 1.30g
static constexpr float    SC_HYSTERESIS_G = 0.40f; // lowLine  = 1.20g
static constexpr uint32_t SC_COOLDOWN_MS  = 265;   // min time between steps, max about 185 SPM

// Save step count to NVS at most once every 5 seconds
static constexpr uint32_t SC_NVS_SAVE_INTERVAL_MS = 3000;

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
    uint32_t _lastSaveMs     = 0;

    bool     _paused         = false;
    bool     _initialised    = false;
};

#endif // STEP_COUNTER_H
// step_counter.h
// Header for the step counter module.
// Handles reading the ADXL335 and counting steps using magnitude peak detection.

#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <Arduino.h>
#include "../calibration/calibration.h"

// Step counter tuning.
// The detector uses acceleration magnitude:
// magnitude = sqrt(x^2 + y^2 + z^2)
//
// At rest, magnitude should sit around 1.0g because of gravity.
// A possible step peak is detected when magnitude rises above:
// 1.0g + SC_THRESHOLD_G
// then drops below:
// 1.0g + SC_HYSTERESIS_G
//
// A peak is not counted immediately. It first becomes a candidate.
// A real step is counted only when another peak arrives within a realistic
// human step timing window. This helps reject isolated taps and vibrations.
static constexpr float    SC_THRESHOLD_G  = 0.22f; // peak must exceed about 1.22g
static constexpr float    SC_HYSTERESIS_G = 0.10f; // must drop below about 1.10g to reset

// Timing filter.
// Too fast = likely vibration/noise.
// Too slow = not part of a real walking rhythm.
static constexpr uint32_t SC_MIN_STEP_INTERVAL_MS = 325;  // max about 185 steps/min
static constexpr uint32_t SC_MIN_STEP_SPM         = 50;   // lower frequency threshold
static constexpr uint32_t SC_MAX_STEP_INTERVAL_MS = 60000UL / SC_MIN_STEP_SPM; // about 1333ms

static constexpr uint32_t SC_NVS_BATCH = 10; // only write to flash every 10 steps to reduce wear

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

    void resetDetectionState();
    void handleStepPeak(uint32_t now, float magnitude);

    Calibration &_cal;

    uint32_t _stepCount    = 0;
    bool     _stepDetected = false;

    // Peak detection state.
    bool _aboveThreshold = false;

    // Rhythm confirmation state.
    bool     _candidateActive     = false;
    uint32_t _candidateStepTimeMs = 0;
    uint32_t _lastStepTimeMs      = 0;

    uint32_t _lastDisplayMs = 0;
    uint32_t _lastSerialMs  = 0;

    bool _initialised = false;
};

#endif // STEP_COUNTER_H
#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <Arduino.h>
#include "../calibration/calibration.h"

// ─────────────────────────────────────────────────────────────
//  ADXL335 pin assignments  (matches calibration.cpp)
// ─────────────────────────────────────────────────────────────
static constexpr int SC_PIN_X = 34;
static constexpr int SC_PIN_Y = 35;
static constexpr int SC_PIN_Z = 32;

// ─────────────────────────────────────────────────────────────
//  ADXL335 analog conversion constants (3.3 V supply)
//    Voltage   = raw * (3.3 / 4095.0)
//    g value   = (voltage - 1.65) / 0.33
//  Where:
//    1.65 V  = mid-supply zero-g bias  (Vcc / 2)
//    0.33 V/g = sensitivity at 3.3 V
// ─────────────────────────────────────────────────────────────
static constexpr float SC_VCC         = 3.3f;
static constexpr float SC_ADC_MAX     = 4095.0f;
static constexpr float SC_ZERO_G_BIAS = 1.65f;   // V  (Vcc / 2)
static constexpr float SC_SENSITIVITY = 0.33f;   // V/g at 3.3 V

// ─────────────────────────────────────────────────────────────
//  Tuning constants
// ─────────────────────────────────────────────────────────────
static constexpr float    SC_THRESHOLD_G  = 0.40f; // must exceed 1g + this to register
static constexpr float    SC_HYSTERESIS_G = 0.16f; // must fall below 1g + this to confirm peak
static constexpr uint32_t SC_COOLDOWN_MS  = 250;   // minimum ms between steps
static constexpr uint32_t SC_NVS_BATCH    = 10;    // save to NVS every N steps

// ─────────────────────────────────────────────────────────────
//  StepCounter class
// ─────────────────────────────────────────────────────────────
class StepCounter {
public:
    // Pass a reference to the Calibration object so we can read offsets
    StepCounter(Calibration &cal);

    // Call once in setup() — configures pins and loads NVS count
    bool begin();

    // Call every 20 ms in loop() — one full sample-and-detect cycle
    void update();

    // Returns the current step count (including unsaved steps)
    uint32_t getStepCount() const;

    // Force an immediate NVS save (e.g. before deep-sleep)
    void saveNow();

    // Reset count to zero and persist to NVS
    void resetCount();

private:
    // ── ADXL335 helpers ──────────────────────────────────────
    void readADXL335(float &x, float &y, float &z);

    // ── NVS helpers ──────────────────────────────────────────
    bool loadFromNVS();
    bool saveToNVS();

    // ── Members ──────────────────────────────────────────────
    Calibration &_cal;

    uint32_t _stepCount      = 0;
    bool     _aboveThreshold = false;
    uint32_t _lastStepTimeMs = 0;

    uint32_t _lastDisplayMs  = 0;
    uint32_t _lastSerialMs   = 0;

    bool     _initialised    = false;
};

#endif // STEP_COUNTER_H
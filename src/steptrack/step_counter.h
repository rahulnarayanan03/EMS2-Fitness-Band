#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
//  Tuning constants — adjust these after physical testing
// ─────────────────────────────────────────────────────────────

// Peak must rise this many g above 1 g (gravity baseline) to register
static constexpr float SC_THRESHOLD_G   = 0.40f;   // 0.40 g

// Peak must fall THIS far below (1g + THRESHOLD) to confirm it finished.
// Using 40 % of threshold gives a clean hysteresis band.
static constexpr float SC_HYSTERESIS_G  = 0.16f;   // 0.16 g  (= THRESHOLD × 0.4)

// Fastest realistic step cadence: 250 ms ≈ 240 steps/min (sprinting)
static constexpr uint32_t SC_COOLDOWN_MS   = 250;

// NVS is written every N steps to limit flash wear
static constexpr uint32_t SC_NVS_BATCH     = 10;

// ADXL345 full-scale sensitivity at ±2 g, 10-bit output → 256 LSB/g
static constexpr float SC_LSB_PER_G        = 256.0f;

// ─────────────────────────────────────────────────────────────
//  StepCounter class
// ─────────────────────────────────────────────────────────────
class StepCounter {
public:
    // Call once in setup() — inits I2C, ADXL345, and loads NVS count
    bool begin();

    // Call every 20 ms in loop() — one complete sample-and-detect cycle
    void update();

    // Return current step count (including unsaved steps)
    uint32_t getStepCount() const;

    // Force an immediate NVS save (e.g. before deep-sleep)
    void saveNow();

    // Reset count to zero and persist to NVS
    void resetCount();

private:
    // ── ADXL345 helpers ──────────────────────────────────────
    bool    initADXL345();
    bool    readADXL345(float &x, float &y, float &z);

    // Write one byte to an ADXL345 register
    void    writeReg(uint8_t reg, uint8_t value);

    // Read one signed byte from an ADXL345 register
    int8_t  readReg(uint8_t reg);

    // ── NVS helpers ──────────────────────────────────────────
    bool     loadFromNVS();
    bool     saveToNVS();

    // ── State ────────────────────────────────────────────────
    uint32_t _stepCount       = 0;
    bool     _aboveThreshold  = false;
    uint32_t _lastStepTimeMs  = 0;

    // Display and serial throttle timers
    uint32_t _lastDisplayMs   = 0;
    uint32_t _lastSerialMs    = 0;

    bool     _initialised     = false;
};

#endif // STEP_COUNTER_H
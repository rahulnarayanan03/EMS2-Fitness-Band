#ifndef PACEFIND_H
#define PACEFIND_H

#include <Arduino.h>
#include <string.h>

#define PACEFIND_SMOOTHING_WINDOW 4

class PACEFIND {
public:
    void update(unsigned long currentTime);
    void checkTimeout(unsigned long currentTime);
    const char* getPace();

private:
    // Reject step events that are too close together.
    // This is a second safety layer after the StepCounter cooldown.
    static const unsigned long MIN_STEP_INTERVAL_MS = 250;

    // If two detected steps are further apart than this, do not treat them
    // as part of the same walking rhythm.
    static const unsigned long MAX_TRACKED_INTERVAL_MS = 3000;

    // If no detected steps arrive for this long, classify as standing.
    // Lower = faster standing animation, but more risk of flicker during slow walking.
    // 3500 is stable. Try 3000 if you want it to return to standing faster.
    static const unsigned long STAND_TIMEOUT_MS = 3500;

    // Lower walking threshold.
    // 50 SPM means max interval of about 1200 ms between detected steps.
    static const unsigned int WALK_SPM_MIN = 50;

    // Running threshold.
    // Normal walking can be around 90 to 120 SPM, so running should not start at 100.
    static const unsigned int RUN_SPM_MIN = 130;

    // Require more than one plausible step before switching to WALKING.
    // 2 means first detected step = candidate, second plausible step = WALKING.
    static const uint8_t WALK_CONFIRM_STEPS = 2;

    // Require repeated fast intervals before switching to RUNNING.
    static const uint8_t RUN_CONFIRM_INTERVALS = 2;

    unsigned long lastStepTime = 0;

    unsigned long stepIntervals[PACEFIND_SMOOTHING_WINDOW] = {};
    int intervalCount = 0;
    int intervalHead  = 0;

    uint8_t walkCandidateSteps    = 0;
    uint8_t runCandidateIntervals = 0;

    const char* currentPace = "STANDING";
    const char* stablePace  = "STANDING";

    void resetIntervals();
    void resetToStanding(bool clearLastStepTime);
    void addInterval(unsigned long interval);
    unsigned long getAverageInterval() const;

    void setStablePace(const char* newPace);
};

#endif
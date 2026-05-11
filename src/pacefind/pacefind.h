#ifndef PACEFIND_H
#define PACEFIND_H

#define PACEFIND_SMOOTHING_WINDOW 4

class PACEFIND {
public:
    void update(unsigned long currentTime);
    void checkTimeout(unsigned long currentTime);
    const char* getPace();

private:
    static const unsigned long MIN_STEP_INTERVAL = 200;
    static const unsigned long MAX_STEP_INTERVAL = 2000;

    // Step-rate thresholds in steps per minute.
    // Normal walking can easily sit around 90 to 120 SPM.
    // So running should not start at 100 SPM.
    static const unsigned int  WALK_SPM_MIN      = 60;
    static const unsigned int  RUN_SPM_MIN       = 130;

    unsigned long lastStepTime = 0;
    unsigned long stepIntervals[PACEFIND_SMOOTHING_WINDOW] = {};
    int           intervalCount  = 0;
    int           intervalHead   = 0;

    const char*   currentPace = "STANDING";
    const char*   stablePace  = "STANDING";
    const char*   pendingPace = "STANDING";
};

#endif
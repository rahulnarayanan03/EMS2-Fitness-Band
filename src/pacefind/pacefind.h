#ifndef PACEFIND_H
#define PACEFIND_H

#define PACEFIND_SMOOTHING_WINDOW 4

class PACEFIND {
public:
    void update(unsigned long currentTime);
    const char* getPace();

private:
    static const unsigned long MIN_STEP_INTERVAL = 200;
    static const unsigned long MAX_STEP_INTERVAL = 2000;
    static const unsigned int  WALK_SPM_MIN      = 70;
    static const unsigned int  RUN_SPM_MIN        = 130;

    unsigned long lastStepTime = 0;
    unsigned long stepIntervals[PACEFIND_SMOOTHING_WINDOW] = {};
    int           intervalCount  = 0;   // number of valid entries in stepIntervals
    int           intervalHead   = 0;   // circular buffer write index

    const char*   currentPace = "STANDING";
    const char*   stablePace  = "STANDING";
    const char*   pendingPace = "STANDING"; // for anti-flicker

protected:
};

#endif

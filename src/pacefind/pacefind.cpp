#include "PACEFIND.h"

// USAGE: call pacefind.update(millis()) in main.cpp ONLY when after a new step is detected (by Karthik's code)!!!!!
void PACEFIND::update(unsigned long currentTime) {
    // This if-statement runs when this function is first called.
    if (lastStepTime == 0) {
        lastStepTime = currentTime;
        return;
    }

    // This interval is the time between each detection, will have to rely on
    // Karthik's detection stability to function properly.
    unsigned long interval = currentTime - lastStepTime;

    if (interval < MIN_STEP_INTERVAL) {
    // Potential noisy detection if this statement is true. This is quite similar to pushbutton debouncing.
    // We return here to stop any further processing.
        return;
    }

    if (interval > MAX_STEP_INTERVAL) {
    // The wearer might very potentially be standing. Reset the pace to standing.
    // In case this function isn't called when the user suddenly halted when running, 
    // we'll have to do another standing check in main.cpp.
        intervalCount = 0;
        intervalHead  = 0;
        currentPace   = "STANDING";
        return;
    }

    // After passing all 2 if-statements, we will register the step to be legit. Karthik/Rahul might have already done this legit
    // check when counting the steps??
    lastStepTime = currentTime;

    // Store interval in circular buffer. 
    // Working principle: Given PACEFIND_SMOOTHING_WINDOW = 4. The intervalHead will keep cycling: 0-1-2-3-0-1-2-3
    // due to the remainder math. If PACEFIND_SMOOTHING_WINDOW = 5, the sequence will be: 0-1-2-3-4-0-1-2-3-4.
    // PACEFIND_SMOOTHING_WINDOW will need to be tuned experimentally to find the most suitable value.
    stepIntervals[intervalHead] = interval;
    intervalHead = (intervalHead + 1) % PACEFIND_SMOOTHING_WINDOW;

    // Since we are not zero-ing the circular buffer, we use this intervalCount variable to keep track of the number of legit
    // elements (When the pace got reverted to standing, we start a new cycle, and this variable will help the program to disregard readings
    // from the previous cycle)
    if (intervalCount < PACEFIND_SMOOTHING_WINDOW) intervalCount++;

    // Compute average step interval
    unsigned long sum = 0;
    for (int i = 0; i < intervalCount; i++) sum += stepIntervals[i];
    unsigned long avgInterval = sum / intervalCount;

    // Convert to steps per minute and classify
    unsigned int spm = (unsigned int)(60000UL / avgInterval);
    if (spm >= RUN_SPM_MIN) {
        currentPace = "RUNNING";
    } else if (spm >= WALK_SPM_MIN) {
        currentPace = "WALKING";
    } else {
        currentPace = "STANDING";
    }

    // Anti-flicker: only set the stablePace after two consecutive matching readings. 
    // The number of consecutive readings can also be experimentally tuned
    if (currentPace == stablePace) {
        pendingPace = currentPace;
    } else if (currentPace == pendingPace) {
        stablePace  = currentPace;
    } else {
        pendingPace = currentPace;
    }
}

// This is for the display to update
const char* PACEFIND::getPace() {
    return stablePace;
}

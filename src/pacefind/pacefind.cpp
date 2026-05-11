#include "pacefind.h"

// Call this only when StepCounter reports a newly detected step.
void PACEFIND::update(unsigned long currentTime) {
    // First detected step after boot or after standing.
    // Do not immediately switch to WALKING, because this could be a vibration.
    if (lastStepTime == 0) {
        lastStepTime = currentTime;

        resetIntervals();

        walkCandidateSteps    = 1;
        runCandidateIntervals = 0;

        currentPace = "STANDING";
        setStablePace("STANDING");

        Serial.println("[PACEFIND] First step candidate -> still STANDING");
        return;
    }

    unsigned long interval = currentTime - lastStepTime;

    // Reject unrealistically short intervals.
    if (interval < MIN_STEP_INTERVAL_MS) {
        Serial.printf("[PACEFIND] Ignored short interval: %lums\n", interval);
        return;
    }

    // This is now the latest accepted step event.
    lastStepTime = currentTime;

    // If the gap is too long, treat this as a new candidate movement,
    // not a continuation of walking.
    if (interval > MAX_TRACKED_INTERVAL_MS) {
        resetIntervals();

        walkCandidateSteps    = 1;
        runCandidateIntervals = 0;

        currentPace = "STANDING";
        setStablePace("STANDING");

        Serial.printf("[PACEFIND] Long gap %lums -> new candidate, still STANDING\n", interval);
        return;
    }

    unsigned int instantSpm = (unsigned int)(60000UL / interval);

    // Lower walking threshold.
    // If the rhythm is too slow, keep it as standing.
    if (instantSpm < WALK_SPM_MIN) {
        resetIntervals();

        walkCandidateSteps    = 1;
        runCandidateIntervals = 0;

        currentPace = "STANDING";
        setStablePace("STANDING");

        Serial.printf("[PACEFIND] Too slow for walking: interval=%lums spm=%u -> STANDING\n",
                      interval,
                      instantSpm);
        return;
    }

    // Passed the lower walking threshold, so this is a plausible walking step.
    addInterval(interval);

    if (walkCandidateSteps < 255) {
        walkCandidateSteps++;
    }

    // Require enough plausible steps before showing WALKING.
    if (walkCandidateSteps < WALK_CONFIRM_STEPS) {
        currentPace = "STANDING";
        setStablePace("STANDING");

        Serial.printf("[PACEFIND] Waiting for walking confirmation: %u/%u\n",
                      walkCandidateSteps,
                      WALK_CONFIRM_STEPS);
        return;
    }

    unsigned long avgInterval = getAverageInterval();

    if (avgInterval == 0) {
        currentPace = "WALKING";
        setStablePace("WALKING");
        return;
    }

    unsigned int avgSpm = (unsigned int)(60000UL / avgInterval);

    // Running needs a fast average step rate and repeated confirmation.
    if (avgSpm >= RUN_SPM_MIN) {
        currentPace = "RUNNING";

        if (runCandidateIntervals < 255) {
            runCandidateIntervals++;
        }

        if (runCandidateIntervals >= RUN_CONFIRM_INTERVALS) {
            setStablePace("RUNNING");
        } else {
            // Movement has been confirmed, but running has not.
            // Keep the animation walking instead of flicking instantly to running.
            setStablePace("WALKING");
        }

    } else {
        runCandidateIntervals = 0;

        currentPace = "WALKING";
        setStablePace("WALKING");
    }

    Serial.printf("[PACEFIND] interval=%lums instant=%uSPM avg=%lums avg=%uSPM pace=%s stable=%s\n",
                  interval,
                  instantSpm,
                  avgInterval,
                  avgSpm,
                  currentPace,
                  stablePace);
}

// Called every loop when no step is detected.
void PACEFIND::checkTimeout(unsigned long currentTime) {
    if (lastStepTime == 0) return;

    if (currentTime - lastStepTime > STAND_TIMEOUT_MS) {
        resetToStanding(true);

        Serial.println("[PACEFIND] No recent steps -> STANDING");
    }
}

const char* PACEFIND::getPace() {
    return stablePace;
}

void PACEFIND::resetIntervals() {
    for (int i = 0; i < PACEFIND_SMOOTHING_WINDOW; i++) {
        stepIntervals[i] = 0;
    }

    intervalCount = 0;
    intervalHead  = 0;
}

void PACEFIND::resetToStanding(bool clearLastStepTime) {
    resetIntervals();

    if (clearLastStepTime) {
        lastStepTime = 0;
    }

    walkCandidateSteps    = 0;
    runCandidateIntervals = 0;

    currentPace = "STANDING";
    setStablePace("STANDING");
}

void PACEFIND::addInterval(unsigned long interval) {
    stepIntervals[intervalHead] = interval;
    intervalHead = (intervalHead + 1) % PACEFIND_SMOOTHING_WINDOW;

    if (intervalCount < PACEFIND_SMOOTHING_WINDOW) {
        intervalCount++;
    }
}

unsigned long PACEFIND::getAverageInterval() const {
    if (intervalCount == 0) return 0;

    unsigned long sum = 0;

    for (int i = 0; i < intervalCount; i++) {
        sum += stepIntervals[i];
    }

    return sum / intervalCount;
}

void PACEFIND::setStablePace(const char* newPace) {
    if (strcmp(stablePace, newPace) == 0) {
        return;
    }

    Serial.printf("[PACEFIND] Pace changed: %s -> %s\n", stablePace, newPace);
    stablePace = newPace;
}
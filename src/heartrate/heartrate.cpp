#include <Arduino.h>
#include "HeartRate.h"

// runs every loop
void HeartRate::update(long irValue) {

    //rising edge (heartbeat peak)
    if (!aboveThreshold && irValue > threshold) {

        aboveThreshold = true;

        unsigned long currentTime = millis();

        // skip first invalid reading
        if (lastBeatTime != 0) {

            unsigned long interval = currentTime - lastBeatTime;

            //ms to bpm conversion
            bpm = 60000 / interval;
        }

        lastBeatTime = currentTime;
    }

    //reset when the signal drops below threshold
    if (irValue < threshold) {
        aboveThreshold = false;
    }
}

//return
int HeartRate::getBPM() {
    return bpm;
}
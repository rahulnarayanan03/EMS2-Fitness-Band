#include <Arduino.h>
#include "HeartRate.h"

void HeartRate::update(long irValue) {

    // no finger
    if (irValue < minIR) {
        bpm = 0;
        aboveThreshold = false;
        lastBeatTime = 0;
        intervalCount = 0;
        bufferIndex = 0;
        movingAvg = 0;
        for (int i = 0; i < AVG_SIZE; i++) irBuffer[i] = 0;
        return;
    }

    // timeout
    if (lastBeatTime != 0 && (millis() - lastBeatTime) > BEAT_TIMEOUT) {
        bpm = 0;
        lastBeatTime = 0;
        intervalCount = 0;
        aboveThreshold = false;
        return;
    }

    // update moving average
    irBuffer[bufferIndex] = irValue;
    bufferIndex = (bufferIndex + 1) % AVG_SIZE;

    long sum = 0;
    for (int i = 0; i < AVG_SIZE; i++) sum += irBuffer[i];
    movingAvg = sum / AVG_SIZE;

    // dynamic threshold - slightly above moving average
    long dynamicThreshold = (long)(movingAvg * THRESHOLD_FACTOR);

    // rising edge - beat peak
    if (!aboveThreshold && irValue > dynamicThreshold) {
        aboveThreshold = true;

        unsigned long currentTime = millis();

        if (lastBeatTime != 0) {
            unsigned long interval = currentTime - lastBeatTime;

            if (interval >= 300 && interval <= 1500) {
                intervals[intervalIndex] = interval;
                intervalIndex = (intervalIndex + 1) % 4;
                if (intervalCount < 4) intervalCount++;

                unsigned long avgSum = 0;
                for (int i = 0; i < intervalCount; i++) {
                    avgSum += intervals[i];
                }
                unsigned long avgInterval = avgSum / intervalCount;
                bpm = 60000 / avgInterval;

                Serial.print("Beat detected! BPM: ");
                Serial.println(bpm);
            }
        }

        lastBeatTime = currentTime;
    }

    // falling edge
    if (aboveThreshold && irValue < dynamicThreshold) {
        aboveThreshold = false;
    }
}

int HeartRate::getBPM() {
    return bpm;
}
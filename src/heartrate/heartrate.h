#ifndef HEARTRATE_H
#define HEARTRATE_H

class HeartRate {

public:
    void update(long irValue);
    int getBPM();

private:
    int bpm = 0;
    unsigned long lastBeatTime = 0;
    bool aboveThreshold = false;

    // moving average
    static const int AVG_SIZE = 16;
    long irBuffer[AVG_SIZE] = {0};
    int bufferIndex = 0;
    long movingAvg = 0;

    // beat intervals
    unsigned long intervals[4] = {0, 0, 0, 0};
    int intervalIndex = 0;
    int intervalCount = 0;

    const long minIR = 20000;
    const unsigned long BEAT_TIMEOUT = 3000;
    const float THRESHOLD_FACTOR = 1.02f; // 2% above moving average = beat
};

#endif
#ifndef HEARTRATE_H
#define HEARTRATE_H

class HeartRate {
    
public:
    void update(long irValue);   // takes IR from your MAX30102
    int getBPM();                // returns calculated BPM

private:
    int bpm = 0;

    unsigned long lastBeatTime = 0;

    bool aboveThreshold = false;

    int threshold = 50000; // base threshold (adjust later)
};

#endif
#ifndef HEARTRATE_H
#define HEARTRATE_H

class HeartRate {
public:
    void update(long irValue);
    int getBPM();

private:
    int bpm = 0;
    const long minIR = 10000;
};

#endif
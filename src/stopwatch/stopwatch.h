#include <Arduino.h>
#include <cmath>

class Stopwatch {
public:
    struct SW_Time {
        int milliseconds;
        int seconds;
        int minutes;
    };

    std::pair<int, int> getPointPosition();
    SW_Time getElapsedTime(unsigned long elapsed_time);
};
#include <Arduino.h>
#include <cmath>

static constexpr int SW_PERIOD_MS = 5000;

class Stopwatch {
public:
    struct SW_Time {
        int milliseconds;
        int seconds;
        int minutes;
    };

    std::pair<int, int> getCirclePosition();
    SW_Time getElapsedTime(unsigned long elapsed_time);
    int getElapsedTimeSeconds(unsigned long elapsed_time);

};
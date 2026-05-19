#include <Arduino.h>
#include <cmath>

static constexpr int SW_PERIOD_MS = 5000;
static constexpr int SW_DELAY = 50;

class Stopwatch {
public:
    // Stopwatch constructor
    Stopwatch(int outer_radius, int thickness, int point_radius, int period_ms);

    // Stopwatch default constructor (no period)
    Stopwatch(int outer_radius, int thickness, int point_radius);

    struct SW_Time {
        int milliseconds;
        int seconds;
        int minutes;
    };

    std::pair<int, int> getCirclePosition();
    SW_Time getElapsedTime();
    float getElapsedTimeSeconds();

private:
    int _outer_radius;
    int _thickness;
    int _point_radius;
    int _period_ms;

    unsigned long _elapsed_time;
};
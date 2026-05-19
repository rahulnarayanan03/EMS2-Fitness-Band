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

    // Returns elapsed time formatted in minutes:seconds:milliseconds
    // Minutes range from 0-59, seconds range from 0-59, milliseconds range from 0-99 (99 = 999)
    struct SW_Time {
        int milliseconds;
        int seconds;
        int minutes;
    };

    // Updates the x,y coordinates of the circle orbiting the stopwatch
    void updateCirclePosition();

    // Returns the x,y coordinates of the circle orbiting the stopwatch
    std::pair<int, int> getCirclePosition();

    // Fills time struct with current elapsed time
    SW_Time updateElapsedTime();

    // Converts elapsed time to seconds
    float getElapsedTimeSeconds();

private:
    int _outer_radius;  // Outer radius of the stopwatch face's circle
    int _thickness;     // Line thickness of the stopwatch face's circle
    int _point_radius;  // Radius of the circle orbiting the stopwatch face
    int _period_ms;     // Period of the circle orbiting the stopwatch face

    unsigned long _elapsed_time;    // Elapsed time in milliseconds

    std::pair<int, int> _point_coords;  // x,y coordinates of the circle orbiting the stopwatch
};
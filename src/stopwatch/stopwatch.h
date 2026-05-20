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

    // Elapsed time formatted in minutes:seconds:milliseconds
    // Minutes range from 0-59, seconds range from 0-59, milliseconds range from 0-99 (99 = 999)
    struct SW_Time {
        int _milliseconds;
        int _seconds;
        int _minutes;
    };

    // State of the stopwatch
    enum SW_State {
        IDLE,       // Before starting the watch or after resetting
        RUNNING,    // After pressing start
        STOPPED     // After pressing stop
    };

    // Updates the x,y coordinates of the circle orbiting the stopwatch
    void updateCirclePosition();

    // Returns the x,y coordinates of the circle orbiting the stopwatch
    std::pair<int, int> getCirclePosition();

    // Updates elapsed time with current elapsed time
    void updateElapsedTime();

    // Formats time in the form minutes:seconds:milliseconds
    void formatTime();

    // Returns elapsed time in milliseconds
    unsigned long getElapsedTime();

    // Returns elapsed time converted to seconds
    float getElapsedTimeSeconds();

    // Returns elapsed time formatted in minutes:seconds:milliseconds
    SW_Time getFormattedTime();

    // Start the stopwatch
    void start_sw();

    // Stop the stopwatch
    void stop_sw();

    // Reset the stopwatch
    void reset_sw();

private:
    int _outer_radius;  // Outer radius of the stopwatch face's circle
    int _thickness;     // Line thickness of the stopwatch face's circle
    int _point_radius;  // Radius of the circle orbiting the stopwatch face
    int _period_ms;     // Period of the circle orbiting the stopwatch face

    std::pair<int, int> _point_coords;  // x,y coordinates of the circle orbiting the stopwatch

    unsigned long _elapsed_time;    // Elapsed time in milliseconds
    SW_Time _formatted_time;        // Elapsed time formatted in minutes:seconds:milliseconds

    SW_State _state;    // Stopwatch state
};
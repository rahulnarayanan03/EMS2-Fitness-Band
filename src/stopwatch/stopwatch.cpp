#include "stopwatch.h"

Stopwatch::Stopwatch(int outer_radius, int thickness, int point_radius) {
    _outer_radius = outer_radius;
    _thickness = thickness;
    _point_radius = point_radius;
    _period_ms = SW_PERIOD_MS;
    _elapsed_time = 0;
}

Stopwatch::Stopwatch(int outer_radius, int thickness, int point_radius, int period_ms) {
    _outer_radius = outer_radius;
    _thickness = thickness;
    _point_radius = point_radius;
    _period_ms = period_ms;
    _elapsed_time = 0;
}

Stopwatch::SW_Time Stopwatch::updateElapsedTime() {
    
}

float Stopwatch::getElapsedTimeSeconds() {
    float elapsed_time_f = static_cast<float>(_elapsed_time);

    return elapsed_time_f / 1000.0f;
}

void Stopwatch::updateCirclePosition() {
    int radius = _outer_radius - (_thickness/2);

    float circle_x;
    float circle_y;
    circle_x = -1*(float)radius*cos((2000.0f*M_PI/_period_ms)*(getElapsedTimeSeconds() + _period_ms/4000.0f));
    circle_y = (float)radius*sin((2000.0f*M_PI/_period_ms)*(getElapsedTimeSeconds() + _period_ms/4000.0f));

    _point_coords.first = static_cast<int>(circle_x);
    _point_coords.second = static_cast<int>(circle_y);
}

std::pair<int, int> Stopwatch::getCirclePosition() {
    return _point_coords;
}
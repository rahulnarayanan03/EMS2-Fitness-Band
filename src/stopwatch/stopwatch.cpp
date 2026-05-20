#include "stopwatch.h"
#include "../display/UI/UI.h"

using namespace SW_Consts;

Stopwatch::Stopwatch(int outer_radius, int thickness, int point_radius) {
    _outer_radius = outer_radius;
    _thickness = thickness;
    _point_radius = point_radius;
    _period_ms = SW_PERIOD_MS;
    _elapsed_time = 0;
    _prev_time = 0;
}

Stopwatch::Stopwatch(int outer_radius, int thickness, int point_radius, int period_ms) {
    _outer_radius = outer_radius;
    _thickness = thickness;
    _point_radius = point_radius;
    _period_ms = period_ms;
    _elapsed_time = 0;
    _prev_time = 0;
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

void Stopwatch::updateElapsedTime() {
    // Only update the time if the watch is running
    if (_state == RUNNING) {
        _elapsed_time = millis() - _prev_time;
        _prev_time = _elapsed_time;
    } else {
        _elapsed_time = _prev_time;
    }
}

void Stopwatch::formatTime() {
    _formatted_time._milliseconds = static_cast<int>((_elapsed_time/10) % 100);
    _formatted_time._seconds = static_cast<int>((_elapsed_time/1000) % 60);
    _formatted_time._minutes = static_cast<int>((_elapsed_time/6000) % 60);
}

unsigned long Stopwatch::getElapsedTime() {
    return _elapsed_time;
}

float Stopwatch::getElapsedTimeSeconds() {
    float elapsed_time_f = static_cast<float>(_elapsed_time);

    return elapsed_time_f / 1000.0f;
}

Stopwatch::SW_Time Stopwatch::getFormattedTime() {
    return _formatted_time;
}

void Stopwatch::setState(SW_State state) {
    _state = state;
}

Stopwatch::SW_State Stopwatch::getState() {
    return _state;
}

void Stopwatch::startSW() {
    SW_State state = RUNNING;
    _state = state;
}

void Stopwatch::stopSW() {
    SW_State state = STOPPED;
    _state = state;
}

void Stopwatch::resetSW() {
    SW_State state = IDLE;
    _state = state;
    _elapsed_time = 0;
    _prev_time = 0;
}

void Stopwatch::updateSW() {
    updateElapsedTime();
    formatTime();
    updateCirclePosition();
}

bool Stopwatch::homeTouched(uint16_t tx, uint16_t ty) {
    return (tx >= SW_BTN_X - SW_BTN_R && tx <= SW_BTN_X + SW_BTN_R && ty >= SW_HOME_Y - SW_BTN_R && ty <= SW_HOME_Y + SW_BTN_R);
}

bool Stopwatch::startStopTouched(uint16_t tx, uint16_t ty) {
    return (tx >= SW_BTN_X - SW_BTN_R && tx <= SW_BTN_X + SW_BTN_R && ty >= START_Y - SW_BTN_R && ty <= START_Y + SW_BTN_R);
}

bool Stopwatch::resetTouched(uint16_t tx, uint16_t ty) {
    return (tx >= SW_BTN_X - SW_BTN_R && tx <= SW_BTN_X + SW_BTN_R && ty >= RESET_Y - SW_BTN_R && ty <= RESET_Y + SW_BTN_R);
}
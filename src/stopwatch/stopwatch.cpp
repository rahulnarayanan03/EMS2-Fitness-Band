#include "stopwatch.h"

Stopwatch::Stopwatch() {
    _period_ms = SW_PERIOD_MS;
    _elapsed_time = 0;
    _prev_time = 0;
    _state = IDLE;
    _point_coords.first = 0;
    _point_coords.second = 0;
}

Stopwatch::Stopwatch(int period_ms) {
    _period_ms = period_ms;
    _elapsed_time = 0;
    _prev_time = 0;
    _state = IDLE;
    _point_coords.first = 0;
    _point_coords.second = 0;
}

void Stopwatch::updateCirclePosition() {
    int radius = SW_OUTER_RADIUS - (SW_THICKNESS/2);

    float circle_x;
    float circle_y;
    circle_x = -1*(float)radius*cos((2000.0f*M_PI/_period_ms)*(getElapsedTimeSeconds() + _period_ms/4000.0f)) + SW_X;
    circle_y = (float)radius*sin((2000.0f*M_PI/_period_ms)*(getElapsedTimeSeconds() + _period_ms/4000.0f)) + SW_Y;

    _point_coords.first = static_cast<int>(circle_x);
    _point_coords.second = static_cast<int>(circle_y);
}

std::pair<int, int> Stopwatch::getCirclePosition() {
    return _point_coords;
}

void Stopwatch::updateElapsedTime() {
    // Only update the time if the watch is running
    if (_state == RUNNING) {
        unsigned long now = millis();
        _elapsed_time += (now - _prev_time);
        _prev_time = now;
    } else {
        return;
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

Stopwatch::SW_State Stopwatch::getState() {
    return _state;
}

void Stopwatch::startSW() {
    _state = RUNNING;
    _prev_time = millis();
}

void Stopwatch::stopSW() {
    if (_state == RUNNING) {
        unsigned long now = millis();
        _elapsed_time += (now - _prev_time);
    }

    _state = STOPPED;
}

void Stopwatch::resetSW() {
    _state = IDLE;
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
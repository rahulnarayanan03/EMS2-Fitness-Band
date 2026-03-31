#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class Display {

public:

    void begin();

    void displayPace(const char* pace);
    void displayBPM(int bpm);
    void displaySteps(int steps);

private:
    TFT_eSPI tft;

    // track last-drawn step count so we only redraw when it changes
    int _lastSteps = -1;

    void clearArea(int x, int y, int w, int h);
};

#endif
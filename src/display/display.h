#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>

class Display {


public:

    void begin();

    void displayPace(const char* pace);
    void displayBPM(int bpm);
    void displaySteps(int steps);

private:
    TFT_eSPI tft;

    void clearArea(int x, int y, int w, int h);
};

#endif
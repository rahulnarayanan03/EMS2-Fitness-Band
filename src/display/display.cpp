#include "display.h"

// portrait: 240px wide x 320px tall

void Display::begin() {
    tft.init();
    tft.setRotation(0); // portrait
    tft.fillScreen(TFT_BLACK);

    // static label - drawn once
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("STEPS", 120, 120, 4);
}

//steps (adxl sensor)
void Display::displaySteps(int steps) {
    if (steps == _lastSteps) return; // skip if unchanged
    _lastSteps = steps;

    clearArea(20, 145, 200, 60);

    char buf[10];
    snprintf(buf, sizeof(buf), "%d", steps);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(buf, 120, 180, 6); // font 6 = large segmented digits
}

//pace (adxl sensor)
void Display::displayPace(const char* pace) {
    // stub - to be implemented
}

//bpm (heartrate sensor)
void Display::displayBPM(int bpm) {
    // stub - to be implemented
}

void Display::clearArea(int x, int y, int w, int h) {
    tft.fillRect(x, y, w, h, TFT_BLACK);
}
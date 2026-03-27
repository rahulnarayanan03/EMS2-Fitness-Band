#include "Display.h"

//initialisation
void Display::begin() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);

    tft.drawString("Display Ready", 30, 100, 2);
}

//pace (adxl sensor)
void Display::displayPace(const char* pace) {
    
}

//bpm (heartrate sensor)
void Display::displayBPM(int bpm) {
    
}

//steps (adxl sensor)
void Display::displaySteps(int steps) {
    
}
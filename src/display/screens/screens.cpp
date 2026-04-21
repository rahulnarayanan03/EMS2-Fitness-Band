/* screens.cpp
 Implements the static screen helpers declared in screens.h. */

#include "screens.h"

// these colour constants come from UI.h via TFT_eSPI, adjust the include if needed
#include "../UI/UI.h"

void drawCalibrationScreen(TFT_eSPI &tft) {
    tft.fillScreen(GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString("CALIBRATION", 10, 10, 2);
    tft.setTextColor(GB_DARK, GB_LIGHTEST);
    tft.drawString("Move the band slowly", 10, 40, 1);
    tft.drawString("in all directions.", 10, 54, 1);
    tft.drawString("Sampling...", 10, 80, 2);
}

void drawCalibrationDone(TFT_eSPI &tft, bool isReentry, uint32_t savedSteps) {
    tft.fillScreen(GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString("Calibration done!", 10, 10, 2);

    if (isReentry) {
        tft.setTextColor(GB_DARK, GB_LIGHTEST);
        tft.drawString("Previous steps:", 10, 50, 1);

        char buf[16];
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)savedSteps);
        tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
        tft.drawString(buf, 10, 64, 2);

        tft.fillRect(10, 110, 100, 40, GB_MID);
        tft.drawRect(10, 110, 100, 40, GB_DARKEST);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(GB_DARKEST, GB_MID);
        tft.drawString("KEEP", 60, 130, 2);

        tft.fillRect(130, 110, 100, 40, GB_LIGHT);
        tft.drawRect(130, 110, 100, 40, GB_DARKEST);
        tft.setTextColor(GB_DARKEST, GB_LIGHT);
        tft.drawString("RESET", 180, 130, 2);
    } else {
        tft.fillRect(70, 140, 100, 40, GB_MID);
        tft.drawRect(70, 140, 100, 40, GB_DARKEST);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(GB_DARKEST, GB_MID);
        tft.drawString("HOME", 120, 160, 2);
    }
}

void drawSCTScreen(TFT_eSPI &tft) {
    tft.fillScreen(GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString("STEP COUNT TEST", 10, 10, 2);
    tft.setTextColor(GB_DARK, GB_LIGHTEST);
    tft.drawString("Live output also on", 10, 40, 1);
    tft.drawString("Serial monitor.", 10, 54, 1);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString("STEPS", 10, 90, 1);

    tft.fillRect(70, 260, 100, 40, GB_MID);
    tft.drawRect(70, 260, 100, 40, GB_DARKEST);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(GB_DARKEST, GB_MID);
    tft.drawString("HOME", 120, 280, 2);
}

void updateSCTScreen(TFT_eSPI &tft, uint32_t stepCount) {
    tft.fillRect(10, 104, 220, 30, GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    char buf[12];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)stepCount);
    tft.drawString(buf, 10, 106, 4);
}

void drawSelfTestScreen(TFT_eSPI &tft, bool passed, const char *resultStr,
                        float dX, float dY, float dZ) {
    tft.fillScreen(GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString("SELF TEST", 10, 10, 2);
    tft.setTextColor(passed ? 0x07E0 : TFT_RED, GB_LIGHTEST);
    tft.drawString(resultStr, 10, 40, 4);
    tft.setTextColor(GB_DARK, GB_LIGHTEST);

    char buf[32];
    snprintf(buf, sizeof(buf), "dX: %.3f g", dX);
    tft.drawString(buf, 10, 90, 2);
    snprintf(buf, sizeof(buf), "dY: %.3f g", dY);
    tft.drawString(buf, 10, 115, 2);
    snprintf(buf, sizeof(buf), "dZ: %.3f g", dZ);
    tft.drawString(buf, 10, 140, 2);

    tft.fillRect(70, 260, 100, 40, GB_MID);
    tft.drawRect(70, 260, 100, 40, GB_DARKEST);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(GB_DARKEST, GB_MID);
    tft.drawString("HOME", 120, 280, 2);
}

void drawStubScreen(TFT_eSPI &tft, const char *title) {
    tft.fillScreen(GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString(title, 10, 10, 2);
    tft.setTextColor(GB_DARK, GB_LIGHTEST);
    tft.drawString("Not yet available.", 10, 40, 1);

    tft.fillRect(70, 260, 100, 40, GB_MID);
    tft.drawRect(70, 260, 100, 40, GB_DARKEST);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(GB_DARKEST, GB_MID);
    tft.drawString("HOME", 120, 280, 2);
}
/* screens.h
 Static screen-drawing helpers for calibration, SCT, self-test, and stub views.
 These just draw -- no state logic lives here. */

#pragma once
#include <TFT_eSPI.h>

// pass the tft instance and any data the screen needs

void drawCalibrationScreen(TFT_eSPI &tft);
void drawCalibrationDone(TFT_eSPI &tft, bool isReentry, uint32_t savedSteps);
void drawCalibrationGuided(TFT_eSPI &tft, const char* dirLabel,
                           bool isSampling, int secsLeft, int dirIndex);
void drawSCTScreen(TFT_eSPI &tft);
void updateSCTScreen(TFT_eSPI &tft, uint32_t stepCount);
void drawSelfTestScreen(TFT_eSPI &tft, bool passed, const char *resultStr,
                        float dX, float dY, float dZ);
void drawStubScreen(TFT_eSPI &tft, const char *title);

// setup wizard screens
void drawSetupWelcome(TFT_eSPI &tft);
void drawSetupQuestion(TFT_eSPI &tft, const char* question,
                       const char* unit, float value, int decimals);
/* screens.h
 Static screen-drawing helpers for calibration, SCT, self-test, and stub views.
 These just draw. No state logic lives here. */

#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "stopwatch/stopwatch.h"

using namespace SW_Consts;

// Stopwatch screen colours
static constexpr uint16_t START_BG = 0x0942;
static constexpr uint16_t START_TEXT = 0x4dad;
static constexpr uint16_t STOP_BG = 0x3061;
static constexpr uint16_t STOP_TEXT = 0xf247;
static constexpr uint16_t RESET_BG = 0x31a6;
static constexpr uint16_t POINT_BG = 0x863c;

// Stopwatch time text position
static constexpr int SW_TIME_Y = 110;
static constexpr int SW_TIME_W = 100;
static constexpr int SW_TIME_H = 32;

void drawCalibrationScreen(TFT_eSPI &tft);
void drawCalibrationDone(TFT_eSPI &tft, bool isReentry, uint32_t savedSteps);
void drawCalibrationGuided(TFT_eSPI &tft, const char *dirLabel,
                           bool isSampling, int secsLeft, int dirIndex);

void drawSWTime(TFT_eSPI &tft, const Stopwatch::SW_Time &t);
void drawSWScreen(TFT_eSPI &tft, Stopwatch &sw);
void updateSWScreen(TFT_eSPI &tft, Stopwatch &sw, uint32_t stepCount);
void eraseSWDot(TFT_eSPI &tft, Stopwatch &sw);
void drawSWDot(TFT_eSPI &tft, Stopwatch &sw);

void drawSelfTestScreen(TFT_eSPI &tft, bool passed, const char *resultStr,
                        float dX, float dY, float dZ);

void drawStubScreen(TFT_eSPI &tft, const char *title);

// setup wizard screens
void drawSetupWelcome(TFT_eSPI &tft);
void drawSetupQuestion(TFT_eSPI &tft, const char *question,
                       const char *unit, float value, int decimals);
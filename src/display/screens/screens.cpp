/* screens.cpp
 Implements the static screen helpers declared in screens.h. */

#include "screens.h"
#include "../UI/UI.h"

static constexpr int16_t HOME_BTN_X = 110;
static constexpr int16_t HOME_BTN_Y = 190;
static constexpr int16_t HOME_BTN_W = 100;
static constexpr int16_t HOME_BTN_H = 40;

// App screen colours
static constexpr uint16_t APP_BG          = TFT_BLACK;
static constexpr uint16_t APP_TEXT        = TFT_WHITE;
static constexpr uint16_t APP_MUTED       = TFT_LIGHTGREY;
static constexpr uint16_t APP_BUTTON      = GB_LIGHTEST;
static constexpr uint16_t APP_BUTTON_TEXT = GB_DARKEST;
static constexpr uint16_t APP_BORDER      = GB_DARKEST;

static void drawHomeButton(TFT_eSPI &tft) {
    tft.fillRect(HOME_BTN_X, HOME_BTN_Y, HOME_BTN_W, HOME_BTN_H, APP_BUTTON);
    tft.drawRect(HOME_BTN_X, HOME_BTN_Y, HOME_BTN_W, HOME_BTN_H, APP_BORDER);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(APP_BUTTON_TEXT, APP_BUTTON);
    tft.drawString("HOME", HOME_BTN_X + HOME_BTN_W / 2,
                   HOME_BTN_Y + HOME_BTN_H / 2, 2);
}

void drawCalibrationScreen(TFT_eSPI &tft) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("CALIBRATION", 10, 10, 4);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString("Follow the on-screen", 10, 52, 2);
    tft.drawString("directions to calibrate.", 10, 74, 2);

    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("Starting...", 10, 112, 4);
}

void drawCalibrationDone(TFT_eSPI &tft, bool isReentry, uint32_t savedSteps) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("Calibration done!", 10, 10, 4);

    if (isReentry) {
        tft.setTextColor(APP_MUTED, APP_BG);
        tft.drawString("Previous steps:", 10, 58, 2);

        char buf[16];
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)savedSteps);

        tft.setTextColor(APP_TEXT, APP_BG);
        tft.drawString(buf, 10, 82, 4);

        tft.fillRect(50, 150, 100, 40, APP_BUTTON);
        tft.drawRect(50, 150, 100, 40, APP_BORDER);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(APP_BUTTON_TEXT, APP_BUTTON);
        tft.drawString("KEEP", 100, 170, 2);

        tft.fillRect(170, 150, 100, 40, APP_BUTTON);
        tft.drawRect(170, 150, 100, 40, APP_BORDER);
        tft.setTextColor(APP_BUTTON_TEXT, APP_BUTTON);
        tft.drawString("RESET", 220, 170, 2);
    } else {
        drawHomeButton(tft);
    }
}

void drawCalibrationGuided(TFT_eSPI &tft, const char* dirLabel,
                           bool isSampling, int secsLeft, int dirIndex) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("CALIBRATION", 10, 10, 4);

    // direction label
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString(dirLabel, 10, 56, 2);

    // status line changes colour depending on whether we're sampling or prepping
    tft.setTextColor(isSampling ? TFT_GREEN : TFT_YELLOW, APP_BG);
    tft.drawString(isSampling ? "Hold still!" : "Get ready...", 10, 80, 2);

    // countdown
    char buf[8];
    snprintf(buf, sizeof(buf), "%ds", secsLeft);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString(buf, 10, 120, 4);

    // e.g. "3 / 6"
    char prog[12];
    snprintf(prog, sizeof(prog), "%d / 6", dirIndex + 1);
    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString(prog, 10, 190, 2);
}

void drawSCTScreen(TFT_eSPI &tft) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("STEP COUNT TEST", 10, 10, 4);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString("Live output also on", 10, 54, 2);
    tft.drawString("Serial monitor.", 10, 76, 2);

    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("STEPS", 10, 112, 2);

    drawHomeButton(tft);
}

void updateSCTScreen(TFT_eSPI &tft, uint32_t stepCount) {
    tft.fillRect(10, 135, 220, 45, APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);

    char buf[12];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)stepCount);
    tft.drawString(buf, 10, 135, 4);
}

void drawSelfTestScreen(TFT_eSPI &tft, bool passed, const char *resultStr,
                        float dX, float dY, float dZ) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("SELF TEST", 10, 10, 4);

    tft.setTextColor(passed ? TFT_GREEN : TFT_RED, APP_BG);
    tft.drawString(resultStr, 10, 54, 4);

    tft.setTextColor(APP_MUTED, APP_BG);

    char buf[32];
    snprintf(buf, sizeof(buf), "dX: %.3f g", dX);
    tft.drawString(buf, 10, 100, 2);

    snprintf(buf, sizeof(buf), "dY: %.3f g", dY);
    tft.drawString(buf, 10, 122, 2);

    snprintf(buf, sizeof(buf), "dZ: %.3f g", dZ);
    tft.drawString(buf, 10, 144, 2);

    drawHomeButton(tft);
}

void drawStubScreen(TFT_eSPI &tft, const char *title) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString(title, 10, 10, 4);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString("Not yet available.", 10, 58, 2);

    drawHomeButton(tft);
}

// ---- setup wizard ----------------------------------------------------------

void drawSetupWelcome(TFT_eSPI &tft) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("Welcome to", 160, 60, 4);
    tft.drawString("EMS2 Fitnessband", 160, 100, 2);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString("Let's set up your profile.", 160, 140, 2);

    // tap anywhere to continue
    tft.setTextColor(APP_BUTTON, APP_BG);
    tft.drawString("Tap to begin", 160, 195, 2);
}

void drawSetupQuestion(TFT_eSPI &tft, const char* question,
                       const char* unit, float value, int decimals) {
    tft.fillScreen(APP_BG);

    // question label at top
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString(question, 160, 30, 2);

    // current value large in centre
    char valBuf[12];
    if (decimals == 0) {
        snprintf(valBuf, sizeof(valBuf), "%d %s", (int)value, unit);
    } else {
        snprintf(valBuf, sizeof(valBuf), "%.1f %s", value, unit);
    }

    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString(valBuf, 160, 100, 4);

    // minus button left
    tft.fillRect(20, 155, 70, 50, APP_BUTTON);
    tft.drawRect(20, 155, 70, 50, APP_BORDER);
    tft.setTextColor(APP_BUTTON_TEXT, APP_BUTTON);
    tft.drawString("-", 55, 180, 4);

    // plus button right
    tft.fillRect(230, 155, 70, 50, APP_BUTTON);
    tft.drawRect(230, 155, 70, 50, APP_BORDER);
    tft.setTextColor(APP_BUTTON_TEXT, APP_BUTTON);
    tft.drawString("+", 265, 180, 4);

    // confirm button centre bottom
    tft.fillRect(110, 155, 100, 50, APP_BUTTON);
    tft.drawRect(110, 155, 100, 50, APP_BORDER);
    tft.setTextColor(APP_BUTTON_TEXT, APP_BUTTON);
    tft.drawString("OK", 160, 180, 2);
}
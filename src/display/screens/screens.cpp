/* screens.cpp
 Implements the static screen helpers declared in screens.h. */

#include "screens.h"
#include "../UI/UI.h"

static constexpr int16_t HOME_BTN_X = 95;
static constexpr int16_t HOME_BTN_Y = 184;
static constexpr int16_t HOME_BTN_W = 130;
static constexpr int16_t HOME_BTN_H = 48;

// Setup/settings buttons. These match the hitboxes in main.cpp.
static constexpr int16_t MINUS_BTN_X = 20;
static constexpr int16_t OK_BTN_X    = 110;
static constexpr int16_t PLUS_BTN_X  = 230;
static constexpr int16_t SETUP_BTN_Y = 155;
static constexpr int16_t SIDE_BTN_W  = 70;
static constexpr int16_t OK_BTN_W    = 100;
static constexpr int16_t SETUP_BTN_H = 50;

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
    tft.drawString("HOME",
                   HOME_BTN_X + HOME_BTN_W / 2,
                   HOME_BTN_Y + HOME_BTN_H / 2,
                   4);
}

static void drawSetupButtons(TFT_eSPI &tft) {
    tft.fillRect(MINUS_BTN_X, SETUP_BTN_Y, SIDE_BTN_W, SETUP_BTN_H, APP_BUTTON);
    tft.drawRect(MINUS_BTN_X, SETUP_BTN_Y, SIDE_BTN_W, SETUP_BTN_H, APP_BORDER);

    tft.fillRect(OK_BTN_X, SETUP_BTN_Y, OK_BTN_W, SETUP_BTN_H, APP_BUTTON);
    tft.drawRect(OK_BTN_X, SETUP_BTN_Y, OK_BTN_W, SETUP_BTN_H, APP_BORDER);

    tft.fillRect(PLUS_BTN_X, SETUP_BTN_Y, SIDE_BTN_W, SETUP_BTN_H, APP_BUTTON);
    tft.drawRect(PLUS_BTN_X, SETUP_BTN_Y, SIDE_BTN_W, SETUP_BTN_H, APP_BORDER);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(APP_BUTTON_TEXT, APP_BUTTON);

    tft.drawString("-", MINUS_BTN_X + SIDE_BTN_W / 2, SETUP_BTN_Y + SETUP_BTN_H / 2, 4);
    tft.drawString("OK", OK_BTN_X + OK_BTN_W / 2, SETUP_BTN_Y + SETUP_BTN_H / 2, 4);
    tft.drawString("+", PLUS_BTN_X + SIDE_BTN_W / 2, SETUP_BTN_Y + SETUP_BTN_H / 2, 4);
}

static void drawQuestionText(TFT_eSPI &tft, const char *question) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);

    if (strcmp(question, "What is your height?") == 0) {
        tft.drawString("What is your", 10, 12, 4);
        tft.drawString("height?", 10, 44, 4);

    } else if (strcmp(question, "What is your weight?") == 0) {
        tft.drawString("What is your", 10, 12, 4);
        tft.drawString("weight?", 10, 44, 4);

    } else if (strcmp(question, "What is your age?") == 0) {
        tft.drawString("What is your", 10, 12, 4);
        tft.drawString("age?", 10, 44, 4);

    } else {
        tft.drawString(question, 10, 18, 4);
    }
}

void drawSetupWelcome(TFT_eSPI &tft) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("SETUP", 10, 18, 4);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString("Tap screen", 10, 78, 4);
    tft.drawString("to begin", 10, 112, 4);
}

void drawSetupQuestion(TFT_eSPI &tft, const char *question,
                       const char *unit, float value, int decimals) {
    tft.fillScreen(APP_BG);

    drawQuestionText(tft, question);

    char valueBuf[24];

    if (decimals <= 0) {
        snprintf(valueBuf, sizeof(valueBuf), "%.0f %s", value, unit);
    } else {
        snprintf(valueBuf, sizeof(valueBuf), "%.1f %s", value, unit);
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString(valueBuf, SCREEN_W / 2, 112, 4);

    drawSetupButtons(tft);
}

void drawCalibrationScreen(TFT_eSPI &tft) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("CALIBRATION", 10, 10, 4);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString("Move band", 10, 58, 4);
    tft.drawString("slowly", 10, 92, 4);

    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("Sampling...", 10, 138, 4);
}

void drawCalibrationGuided(TFT_eSPI &tft, const char *dirLabel,
                           bool isSampling, int secsLeft, int dirIndex) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);

    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("CALIBRATION", 10, 10, 4);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString(dirLabel, 10, 58, 4);

    tft.setTextColor(isSampling ? TFT_GREEN : TFT_ORANGE, APP_BG);
    tft.drawString(isSampling ? "Hold still!" : "Get ready", 10, 96, 4);

    char timeBuf[12];
    snprintf(timeBuf, sizeof(timeBuf), "%ds", secsLeft);

    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString(timeBuf, 10, 138, 4);

    char stepBuf[12];
    snprintf(stepBuf, sizeof(stepBuf), "%d / 6", dirIndex + 1);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString(stepBuf, 10, 194, 4);
}

void drawCalibrationDone(TFT_eSPI &tft, bool isReentry, uint32_t savedSteps) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("CALIBRATION DONE", 10, 10, 4);
    tft.drawString("", 10, 48, 4);

    if (isReentry) {
        tft.setTextColor(APP_MUTED, APP_BG);
        tft.drawString("Old steps:", 10, 90, 4);

        char buf[16];
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)savedSteps);

        tft.setTextColor(APP_TEXT, APP_BG);
        tft.drawString(buf, 10, 126, 4);

        tft.fillRect(35, 184, 115, 48, APP_BUTTON);
        tft.drawRect(35, 184, 115, 48, APP_BORDER);

        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(APP_BUTTON_TEXT, APP_BUTTON);
        tft.drawString("KEEP", 92, 208, 4);

        tft.fillRect(170, 184, 115, 48, APP_BUTTON);
        tft.drawRect(170, 184, 115, 48, APP_BORDER);
        tft.drawString("RESET", 227, 208, 4);

    } else {
        drawHomeButton(tft);
    }
}

void drawSCTScreen(TFT_eSPI &tft) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("STEP COUNT TEST", 10, 10, 4);
    tft.drawString("", 10, 46, 4);

    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("STEPS", 10, 120, 4);

    drawHomeButton(tft);
}

void updateSCTScreen(TFT_eSPI &tft, uint32_t stepCount) {
    // Clear only the step number area.
    // Do not clear over the HOME button.
    tft.fillRect(10, 154, 80, 28, APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);

    char buf[12];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)stepCount);

    tft.drawString(buf, 10, 154, 4);
}

void drawSelfTestScreen(TFT_eSPI &tft, bool passed, const char *resultStr,
                        float dX, float dY, float dZ) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("SELF TEST", 10, 10, 4);

    tft.setTextColor(passed ? TFT_GREEN : TFT_RED, APP_BG);
    tft.drawString(resultStr, 10, 50, 4);

    tft.setTextColor(APP_MUTED, APP_BG);

    char buf[32];

    snprintf(buf, sizeof(buf), "dX %.2f g", dX);
    tft.drawString(buf, 10, 92, 4);

    snprintf(buf, sizeof(buf), "dY %.2f g", dY);
    tft.drawString(buf, 10, 126, 4);

    snprintf(buf, sizeof(buf), "dZ %.2f g", dZ);
    tft.drawString(buf, 10, 160, 4);

    drawHomeButton(tft);
}

void drawStubScreen(TFT_eSPI &tft, const char *title) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString(title, 10, 10, 4);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString("Not available", 10, 58, 4);

    drawHomeButton(tft);
}
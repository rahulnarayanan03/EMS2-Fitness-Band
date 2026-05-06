// UI.h
// Homepage for the EMS2 fitness band.
//
// Owns the full home screen rendering including Red's animated sprite.
// Animation is driven internally - no need to call animM separately from main.

#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>
#include "../animation/anim_data.h"
#include "../../steptrack/step_counter.h"
#include "../../calibration/calibration.h"

// battery config - swap BATT_ADC_PIN and BATT_DIVIDER_RATIO once PCB schematic confirmed
static constexpr uint8_t  BATT_ADC_PIN        = 34;
static constexpr float    BATT_DIVIDER_RATIO  = 2.0f;
static constexpr float    BATT_MAX_V          = 4.2f;
static constexpr float    BATT_MIN_V          = 3.3f;
static constexpr float    BATT_USB_THRESHOLD  = 4.25f;

// how long each GIF frame is shown (ms)
static constexpr uint32_t UI_WALK_FRAME_MS = 120;
static constexpr uint32_t UI_RUN_FRAME_MS  = 80;

// GB palette RGB565
static constexpr uint16_t GB_DARKEST  = 0x1923;
static constexpr uint16_t GB_DARK     = 0x2945;
static constexpr uint16_t GB_MID      = 0x5360;
static constexpr uint16_t GB_LIGHT    = 0x8C10;
static constexpr uint16_t GB_LIGHTEST = 0xC710;
static constexpr uint16_t GB_INACTIVE = 0x9D12;
static constexpr uint16_t HEART_RED   = 0xD020;

// display orientation and layout geometry
static constexpr uint8_t  DISPLAY_ROTATION = 3;   // 90 degrees counter-clockwise
static constexpr int16_t  SCREEN_W         = 320;
static constexpr int16_t  SCREEN_H         = 240;

static constexpr int16_t TOPBAR_H = 20;

//layout constants
static constexpr int16_t SPRITE_X = 215;
static constexpr int16_t SPRITE_Y = 56;
static constexpr int16_t SPRITE_W = 65;
static constexpr int16_t SPRITE_H = 75;

static constexpr int16_t STEPS_BAR_X = 178;
static constexpr int16_t STEPS_BAR_Y = 150;
static constexpr int16_t STEPS_BAR_W = 130;
static constexpr int16_t STEPS_BAR_H = 70;

static constexpr int16_t LEFT_PNL_X = 8;
static constexpr int16_t LEFT_PNL_Y = 150;
static constexpr int16_t LEFT_PNL_W = 112;
static constexpr int16_t LEFT_PNL_H = 70;

static constexpr int16_t RIGHT_PNL_X = 8;
static constexpr int16_t RIGHT_PNL_Y = 24;
static constexpr int16_t RIGHT_PNL_W = 150;
static constexpr int16_t RIGHT_PNL_H = 112;

static constexpr int16_t BTN_W   = 68;
static constexpr int16_t BTN_H   = 45;
static constexpr int16_t BTN_GAP = 6;

enum class UIActivity { NONE, STANDING, WALKING, RUNNING };

class UI {
public:
    UI(TFT_eSPI &tft, StepCounter &stepM, Calibration &cal);

    void begin();
    void update(uint32_t nowMs, float cv, float cp);

    void setTime(uint8_t hour, uint8_t minute);
    void setDate(uint8_t day, uint8_t month);
    void setBPM(int bpm);
    void setPace(const char *pace);

    bool checkButtonTouch(uint16_t tx, uint16_t ty, uint8_t &btnIndex);

private:
    TFT_eSPI    &_tft;
    StepCounter &_stepM;
    Calibration &_cal;

    AnimatedGIF _gif;

    UIActivity _activity     = UIActivity::NONE;
    UIActivity _lastActivity = UIActivity::NONE;
    int        _gifFrame     = 0;
    uint32_t   _lastFrameMs  = 0;

    const char *_pace = "STANDING";

    uint8_t  _lastHour    = 255;
    uint8_t  _lastMinute  = 255;
    uint8_t  _lastDay     = 255;
    uint8_t  _lastMonth   = 255;
    uint32_t _lastSteps   = 0xFFFFFFFF;
    int8_t   _lastBattPct = -2;
    uint16_t _lastBattMv  = 0;
    int      _lastBPM     = -2;

    uint8_t _hour   = 0;
    uint8_t _minute = 0;
    uint8_t _day    = 1;
    uint8_t _month  = 1;
    int     _bpm    = -1;

    void drawStaticLayout();
    void drawTopBar();
    void drawStepsBar();
    void drawLeftPanel();
    void drawRightPanel();
    void drawButton(int16_t x, int16_t y, const char *label, bool active);
    void drawHeartIcon(int16_t cx, int16_t cy);
    void drawBattIcon(int16_t x, int16_t y, int8_t pct);

    void refreshTime();
    void refreshDate();
    void refreshSteps(uint32_t steps);
    void refreshBPM();
    void refreshBattery(float cv, float cp);

    void updateActivity();
    void advanceSprite(uint32_t nowMs);
    void drawStandingGif();
    void drawGifFrame(const uint8_t *data, size_t len,
                      uint32_t frameMs, uint32_t nowMs);

    float  readBatteryVoltage();
    int8_t readBatteryPercent(float vBatt);

    static void gifDraw(GIFDRAW *pDraw);
    static UI  *_instance;
};

#endif // UI_H
// UI.h
// Homepage for the EMS2 fitness band.
//
// Owns the full home screen rendering including Red's animated sprite.
// Animation is driven internally - no need to call animM separately from main.
//
// Screen layout (240x320 portrait):
//   y=0   - y=20   time bar (HH:MM)
//   y=26  - y=100  Red's sprite centred (65x75), animated by step rate
//   y=114 - y=158  steps bar: count left, date bottom-right
//   y=164 - y=244  bottom row: left panel (heart + battery) | right panel (2x2 app buttons)
//
// State machine cases (defined in main.cpp):
//   Case 1 = C.T (calibration - boots here)
//   Case 2 = Homepage (this file)
//   Case 3 = S.T
//   Case 4 = S.C.T
//   Case 5 = P.ID.T

#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>
#include "../animation/anim_data.h"
#include "../../steptrack/step_counter.h"
#include "../../calibration/calibration.h"

// battery config - swap BATT_ADC_PIN and BATT_DIVIDER_RATIO once PCB schematic confirmed
static constexpr uint8_t  BATT_ADC_PIN       = 34;
static constexpr float    BATT_DIVIDER_RATIO  = 2.0f;
static constexpr float    BATT_MAX_V          = 4.2f;
static constexpr float    BATT_MIN_V          = 3.3f;
static constexpr float    BATT_USB_THRESHOLD  = 4.25f;

// how long each GIF frame is shown (ms)
static constexpr uint32_t UI_WALK_FRAME_MS    = 120;
static constexpr uint32_t UI_RUN_FRAME_MS     = 80;

// GB palette RGB565
static constexpr uint16_t GB_DARKEST  = 0x1923; // #304828
static constexpr uint16_t GB_DARK     = 0x2945; // #506050
static constexpr uint16_t GB_MID      = 0x5360; // #88A860
static constexpr uint16_t GB_LIGHT    = 0x8C10; // #A8C880
static constexpr uint16_t GB_LIGHTEST = 0xC710; // #C8D8A8
static constexpr uint16_t GB_INACTIVE = 0x9D12; // #B8CCA0
static constexpr uint16_t HEART_RED   = 0xD020; // #D04040

// layout geometry
static constexpr int16_t TOPBAR_H      = 20;
static constexpr int16_t SPRITE_X      = 88;
static constexpr int16_t SPRITE_Y      = 26;
static constexpr int16_t SPRITE_W      = 65;
static constexpr int16_t SPRITE_H      = 75;
static constexpr int16_t STEPS_BAR_X   = 4;
static constexpr int16_t STEPS_BAR_Y   = 114;
static constexpr int16_t STEPS_BAR_W   = 232;
static constexpr int16_t STEPS_BAR_H   = 44;
static constexpr int16_t BOTTOM_Y      = 236;  // panels sit near the screen bottom (320 - 80 - 4)
static constexpr int16_t BOTTOM_H      = 80;
static constexpr int16_t LEFT_PNL_X    = 4;
static constexpr int16_t LEFT_PNL_W    = 112;
static constexpr int16_t RIGHT_PNL_X   = 122;
static constexpr int16_t RIGHT_PNL_W   = 114;
static constexpr int16_t BTN_W         = 50;
static constexpr int16_t BTN_H         = 34;
static constexpr int16_t BTN_GAP       = 4;

enum class UIActivity { NONE, STANDING, WALKING, RUNNING };

class UI {
public:
    UI(TFT_eSPI &tft, StepCounter &stepM, Calibration &cal);

    void begin();
    void update(uint32_t nowMs);

    void setTime(uint8_t hour, uint8_t minute);
    void setDate(uint8_t day, uint8_t month);
    void setBPM(int bpm);        // pass -1 to show "--"
    void setPace(const char *pace);  // pass "STANDING", "WALKING", or "RUNNING" from pacefind

    bool checkButtonTouch(uint16_t tx, uint16_t ty, uint8_t &btnIndex);

private:
    TFT_eSPI    &_tft;
    StepCounter &_stepM;
    Calibration &_cal;

    AnimatedGIF  _gif;

    // sprite / activity state
    UIActivity   _activity        = UIActivity::NONE;
    UIActivity   _lastActivity    = UIActivity::NONE;
    int          _gifFrame        = 0;
    uint32_t     _lastFrameMs     = 0;

    // pace string from pacefind ("STANDING", "WALKING", "RUNNING")
    const char  *_pace            = "STANDING";

    // dirty tracking for each display region
    uint8_t      _lastHour        = 255;
    uint8_t      _lastMinute      = 255;
    uint8_t      _lastDay         = 255;
    uint8_t      _lastMonth       = 255;
    uint32_t     _lastSteps       = 0xFFFFFFFF;
    int8_t       _lastBattPct     = -2;
    int          _lastBPM         = -2;

    // pending values set via setters
    uint8_t      _hour   = 0;
    uint8_t      _minute = 0;
    uint8_t      _day    = 1;
    uint8_t      _month  = 1;
    int          _bpm    = -1;

    // drawing
    void drawStaticLayout();
    void drawTopBar();
    void drawStepsBar();
    void drawLeftPanel();
    void drawRightPanel();
    void drawButton(int16_t x, int16_t y, const char *label, bool active);
    void drawHeartIcon(int16_t cx, int16_t cy);
    void drawBattIcon(int16_t x, int16_t y, int8_t pct);

    // region updaters (all use dirty tracking)
    void refreshTime();
    void refreshDate();
    void refreshSteps(uint32_t steps);
    void refreshBPM();
    void refreshBattery();

    // sprite animation
    void updateActivity();
    void advanceSprite(uint32_t nowMs);
    void drawStandingGif();
    void drawGifFrame(const uint8_t *data, size_t len,
                      uint32_t frameMs, uint32_t nowMs);

    // battery ADC
    int8_t readBatteryPercent();

    // static callback for AnimatedGIF
    static void     gifDraw(GIFDRAW *pDraw);
    static UI      *_instance;
};

#endif // UI_H
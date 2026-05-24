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
#include "../icons/fire.h"
#include "../icons/axes.h"
#include "../icons/chip.h"
#include "../icons/pace.h"
#include "../icons/sw.h"

// battery config - swap BATT_ADC_PIN and BATT_DIVIDER_RATIO once PCB schematic confirmed
static constexpr uint8_t  BATT_ADC_PIN       = 34;
static constexpr float    BATT_DIVIDER_RATIO = 2.0f;
static constexpr float    BATT_MAX_V         = 4.2f;
static constexpr float    BATT_MIN_V         = 3.3f;
static constexpr float    BATT_USB_THRESHOLD = 4.25f;

// how long each GIF frame is shown, in ms
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
static constexpr uint16_t LEAF_GREEN = 0x5DE7;
static constexpr uint16_t APP_GLARE = 0x964f;

// Green retro button colours
static constexpr uint16_t GB_BUTTON = 0x74c6;
static constexpr uint16_t BTN_SHADOW = 0x63e4;
static constexpr uint16_t BTN_GLARE = 0x8da7;
static constexpr uint16_t INACTIVE_BUTTON = 0xa5ed;
static constexpr uint16_t INACTIVE_SHADOW = 0x8cea;
static constexpr uint16_t INACTIVE_GLARE = 0xc711;

// Retro reset button colours
static constexpr uint16_t RESET_RED  = 0xD000;
static constexpr uint16_t RESET_SHADOW  = 0x9800;
static constexpr uint16_t RESET_GLARE  = 0xd2eb;
static constexpr uint16_t RESET_DARK = 0x7000;
static constexpr uint16_t RESET_TEXT = TFT_WHITE;

// display orientation and layout geometry
static constexpr uint8_t  DISPLAY_ROTATION = 3;
static constexpr int16_t  SCREEN_W         = 320;
static constexpr int16_t  SCREEN_H         = 240;

static constexpr int16_t TOPBAR_H = 34;

// gear icon
static constexpr int16_t GEAR_X = 300;
static constexpr int16_t GEAR_Y = 17;
static constexpr int16_t GEAR_HIT_X = 282;
static constexpr int16_t GEAR_HIT_W = 36;

// layout constants
static constexpr int16_t SPRITE_X = 235;
static constexpr int16_t SPRITE_Y = 62;
static constexpr int16_t SPRITE_W = 70;
static constexpr int16_t SPRITE_H = 78;

static constexpr int16_t STEPS_BAR_X = 176;
static constexpr int16_t STEPS_BAR_Y = 158;
static constexpr int16_t STEPS_BAR_W = 136;
static constexpr int16_t STEPS_BAR_H = 72;

static constexpr int16_t STEP_RESET_BTN_W = 68;
static constexpr int16_t STEP_RESET_BTN_H = 34;
static constexpr int16_t STEP_RESET_BTN_X = STEPS_BAR_X + STEPS_BAR_W - STEP_RESET_BTN_W - 7;
static constexpr int16_t STEP_RESET_BTN_Y = STEPS_BAR_Y + 16;

static constexpr int16_t LEFT_PNL_X = 8;
static constexpr int16_t LEFT_PNL_Y = 158;
static constexpr int16_t LEFT_PNL_W = 160;
static constexpr int16_t LEFT_PNL_H = 72;

static constexpr int16_t RIGHT_PNL_X = 8;
static constexpr int16_t RIGHT_PNL_Y = 40;
static constexpr int16_t RIGHT_PNL_W = 198;
static constexpr int16_t RIGHT_PNL_H = 106;

static constexpr int16_t BTN_W   = 52;
static constexpr int16_t BTN_H   = 52;
static constexpr int16_t BTN_GAP = 8;

// Stopwatch constants namespace
namespace SW_Consts {
    static constexpr int16_t SW_X = 120;        // X coordinate of stopwatch circle midpoint
    static constexpr int16_t SW_Y = 120;        // Y coordinate of stopwatch circle midpoint
    static constexpr int16_t SW_OUTER_RADIUS = 105;     // Outer radius of stopwatch circle
    static constexpr int16_t SW_THICKNESS = 5;  // Line thickness of stopwatch circle
    static constexpr int16_t SW_POINT_R = 10;   // Radius of the circle orbiting the stopwatch face
    static constexpr int16_t SW_BTN_GAP = 8;    // Vertical gap between the buttons
    static constexpr int16_t SW_BTN_R = 35;     // Radius of the buttons
    static constexpr int16_t SW_BTN_X = 275;    // X coordinate of the buttons
    static constexpr int16_t START_Y = 43;      // Y coordinate of start button
    static constexpr int16_t RESET_Y = 121;     // Y coordinate of reset button
    static constexpr int16_t SW_HOME_Y = 199;   // Y coordinate of home button
}

enum class UIActivity { NONE, STANDING, WALKING, RUNNING };

class UI {
public:
    UI(TFT_eSPI &tft, StepCounter &stepM, Calibration &cal);

    void begin();
    void update(uint32_t nowMs, float cv, float cp);

    void setTime(uint8_t hour, uint8_t minute);
    void setDate(uint8_t day, uint8_t month);
    void setPace(const char *pace);
    void setCalories(float kcal);

    bool checkButtonTouch(uint16_t tx, uint16_t ty, uint8_t &btnIndex);
    bool checkStepResetTouch(uint16_t tx, uint16_t ty) const;
    bool checkSettingsTouch(uint16_t tx, uint16_t ty) const;

    void drawStepResetButton();
    void drawStepResetButtonPressed();

    void drawRetroButton(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, int16_t shadow_h, const String &string, int font_size,
                        uint16_t bg_colour, uint16_t b_colour, uint16_t s_colour, uint16_t g_colour, uint16_t t_colour);

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

    uint8_t  _lastHour     = 255;
    uint8_t  _lastMinute   = 255;
    uint8_t  _lastDay      = 255;
    uint8_t  _lastMonth    = 255;
    uint32_t _lastSteps    = 0xFFFFFFFF;
    int8_t   _lastBattPct  = -2;
    uint16_t _lastBattMv   = 0;
    float    _lastCalories = -1.0f;

    uint8_t  _hour     = 0;
    uint8_t  _minute   = 0;
    uint8_t  _day      = 1;
    uint8_t  _month    = 1;
    float    _calories = 0.0f;

    void drawStaticLayout();
    void drawAppIcons();
    void drawTopBar();
    void drawGearIcon();
    void drawStepsBar();
    void drawLeftPanel();
    void drawRightPanel();
    void drawRightPanelNoFill();
    void drawButton(int16_t x, int16_t y, const char *label, bool active);
    void drawButtonBorder(int16_t x, int16_t y, bool active);
    void drawHeartIcon(int16_t cx, int16_t cy);
    void drawBattIcon(int16_t x, int16_t y, int8_t pct);

    void refreshTime();
    void refreshDate();
    void refreshSteps(uint32_t steps);
    void refreshCalories();
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

#endif
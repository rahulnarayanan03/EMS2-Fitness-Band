// healthapp.cpp
// See healthapp.h for overview.

#include "healthapp.h"

// NVS keys
static constexpr char NVS_NS[]      = "healthgoals";
static constexpr char NVS_KCAL[]    = "goalKcal";
static constexpr char NVS_STEPS[]   = "goalSteps";
static constexpr char NVS_MINUTES[] = "goalMins";

// ---- layout -----------------------------------------------------------------

// home button on the menu screen
static constexpr int16_t H_HOME_X = 95;
static constexpr int16_t H_HOME_Y = 178;
static constexpr int16_t H_HOME_W = 130;
static constexpr int16_t H_HOME_H = 48;

// menu screen buttons (set goals / my progress)
static constexpr int16_t MENU_BTN_X  = 40;
static constexpr int16_t MENU_BTN_W  = 240;
static constexpr int16_t MENU_BTN_H  = 54;
static constexpr int16_t MENU_SET_Y  = 42;
static constexpr int16_t MENU_PROG_Y = 112;

// goals screen rows
// 3 rows of h=38 with gap=6 -> total = 36 + 3*38 + 2*6 = 162px, leaving room for save
static constexpr int16_t GOAL_ROW_H   = 38;
static constexpr int16_t GOAL_ROW_GAP = 6;
static constexpr int16_t GOAL_ROW1_Y  = 36;
static constexpr int16_t GOAL_ROW2_Y  = GOAL_ROW1_Y + GOAL_ROW_H + GOAL_ROW_GAP;
static constexpr int16_t GOAL_ROW3_Y  = GOAL_ROW2_Y + GOAL_ROW_H + GOAL_ROW_GAP;

static constexpr int16_t GOAL_MINUS_X = 4;
static constexpr int16_t GOAL_PLUS_X  = 266;
static constexpr int16_t GOAL_SIDE_W  = 48;
static constexpr int16_t GOAL_VAL_X   = 56;
static constexpr int16_t GOAL_VAL_W   = 208;

// save button - 16px gap below last row (ends at ~162), ends at 220
static constexpr int16_t SAVE_BTN_X = 95;
static constexpr int16_t SAVE_BTN_Y = 178;
static constexpr int16_t SAVE_BTN_W = 130;
static constexpr int16_t SAVE_BTN_H = 42;

// progress screen bars - taller bars (32px) for easier reading
// bar3 ends at 154+32=186, back button at 194
static constexpr int16_t BAR_X        = 10;
static constexpr int16_t BAR_W        = 300;
static constexpr int16_t BAR_H        = 32;
static constexpr int16_t BAR_LABEL1_Y = 32;
static constexpr int16_t BAR1_Y       = 46;
static constexpr int16_t BAR_LABEL2_Y = 86;
static constexpr int16_t BAR2_Y       = 100;
static constexpr int16_t BAR_LABEL3_Y = 140;
static constexpr int16_t BAR3_Y       = 154;

static constexpr int16_t BACK_BTN_X = 95;
static constexpr int16_t BACK_BTN_Y = 194;
static constexpr int16_t BACK_BTN_W = 130;
static constexpr int16_t BACK_BTN_H = 40;

// colours - darker fills so white text is always readable on top
static constexpr uint16_t H_BG       = TFT_BLACK;
static constexpr uint16_t H_TEXT     = TFT_WHITE;
static constexpr uint16_t H_MUTED    = TFT_LIGHTGREY;
static constexpr uint16_t H_FILL_CAL = 0xC960;  // dark burnt orange
static constexpr uint16_t H_FILL_STP = 0x03E0;  // dark green - white readable
static constexpr uint16_t H_FILL_MIN = 0x0018;  // dark blue - white readable
static constexpr uint16_t H_BAR_BG   = 0x2945;  // dark background for bars/rows
static constexpr uint16_t H_SEL      = 0xFFE0;  // yellow outline on selected row

// ---- constructor ------------------------------------------------------------

HealthApp::HealthApp(TFT_eSPI &tft, UI &ui)
    : _tft(tft), _ui(ui) {}

// ---- public -----------------------------------------------------------------

void HealthApp::begin() {
    loadGoals();

    _screen    = HealthScreen::HEALTH_MENU;
    _wantsHome = false;
    _field     = GoalField::FIELD_CALORIES;

    drawMenu();
}

void HealthApp::update(uint16_t tx, uint16_t ty, bool touched) {
    if (!touched) return;

    switch (_screen) {
        case HealthScreen::HEALTH_MENU:
            handleMenuTouch(tx, ty);
            break;
        case HealthScreen::HEALTH_GOALS:
            handleGoalsTouch(tx, ty);
            break;
        case HealthScreen::HEALTH_PROGRESS:
            handleProgressTouch(tx, ty);
            break;
    }
}

void HealthApp::resetProgress() {
    _curSteps    = 0;
    _curCalories = 0.0f;
    _curMinutes  = 0;
    _runAccumMs  = 0;
    _wasRunning  = false;
    _runStartMs  = 0;

    // redraw bars straight away if user is looking at the progress screen
    if (_screen == HealthScreen::HEALTH_PROGRESS) {
        drawProgressScreen();
    }
}

void HealthApp::tickActivity(bool isRunning, uint32_t nowMs) {
    if (isRunning && !_wasRunning) {
        // just started running, note the start time
        _runStartMs = nowMs;
        _wasRunning = true;

    } else if (!isRunning && _wasRunning) {
        // stopped running, bank the elapsed time
        _runAccumMs += (nowMs - _runStartMs);
        _wasRunning  = false;
        _curMinutes  = _runAccumMs / 60000;
    }

    // update live while still running
    if (_wasRunning) {
        _curMinutes = (_runAccumMs + (nowMs - _runStartMs)) / 60000;
    }
}

void HealthApp::setCurrentSteps(uint32_t steps) {
    _curSteps = steps;
}

void HealthApp::setCurrentCalories(float kcal) {
    _curCalories = kcal;
}

bool HealthApp::wantsHome() const {
    return _wantsHome;
}

void HealthApp::clearHomeFlag() {
    _wantsHome = false;
}

// ---- NVS --------------------------------------------------------------------

void HealthApp::loadGoals() {
    Preferences prefs;

    if (!prefs.begin(NVS_NS, true)) {
        // nothing saved yet, defaults stay
        return;
    }

    _goalCalories = prefs.getUInt(NVS_KCAL,     500);
    _goalSteps    = prefs.getUInt(NVS_STEPS,   10000);
    _goalMinutes  = prefs.getUInt(NVS_MINUTES,    30);

    prefs.end();

    Serial.printf("[HealthApp] loaded goals - kcal=%u steps=%u mins=%u\n",
                  _goalCalories, _goalSteps, _goalMinutes);
}

void HealthApp::saveGoals() {
    Preferences prefs;

    if (!prefs.begin(NVS_NS, false)) {
        Serial.println("[HealthApp] ERROR: couldn't open NVS for writing");
        return;
    }

    prefs.putUInt(NVS_KCAL,     _goalCalories);
    prefs.putUInt(NVS_STEPS,    _goalSteps);
    prefs.putUInt(NVS_MINUTES,  _goalMinutes);

    prefs.end();

    Serial.printf("[HealthApp] saved goals - kcal=%u steps=%u mins=%u\n",
                  _goalCalories, _goalSteps, _goalMinutes);
}

// ---- drawing ----------------------------------------------------------------

void HealthApp::drawMenu() {
    _tft.fillScreen(H_BG);

    _tft.setTextDatum(TC_DATUM);
    _tft.setTextColor(H_TEXT, H_BG);
    _tft.drawString("HEALTH", 160, 10, 4);

    _ui.drawRetroButton(MENU_BTN_X, MENU_SET_Y, MENU_BTN_W, MENU_BTN_H, 8, 6,
                        "SET GOALS", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);

    _ui.drawRetroButton(MENU_BTN_X, MENU_PROG_Y, MENU_BTN_W, MENU_BTN_H, 8, 6,
                        "MY PROGRESS", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);

    _ui.drawRetroButton(H_HOME_X, H_HOME_Y, H_HOME_W, H_HOME_H, 10, 6,
                        "HOME", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
}

void HealthApp::drawGoalsScreen() {
    _tft.fillScreen(H_BG);

    _tft.setTextDatum(TC_DATUM);
    _tft.setTextColor(H_TEXT, H_BG);
    _tft.drawString("SET GOALS", 160, 10, 4);

    for (int i = 0; i < 3; i++) {
        GoalField f = (GoalField)i;
        int16_t   y = fieldY(f);

        // row background
        _tft.fillRect(GOAL_MINUS_X, y, 316, GOAL_ROW_H, H_BAR_BG);

        // minus button (red)
        _ui.drawRetroButton(GOAL_MINUS_X, y, GOAL_SIDE_W, GOAL_ROW_H, 4, 3, "-", 4,
                            RESET_RED, TFT_BLACK, RESET_SHADOW, RESET_GLARE, TFT_WHITE);

        // plus button (green)
        _ui.drawRetroButton(GOAL_PLUS_X, y, GOAL_SIDE_W, GOAL_ROW_H, 4, 3, "+", 4,
                            GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_BLACK);

        drawGoalValue(f);
        drawFieldHighlight(f, f == _field);
    }

    _ui.drawRetroButton(SAVE_BTN_X, SAVE_BTN_Y, SAVE_BTN_W, SAVE_BTN_H, 10, 5,
                        "SAVE", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_BLACK);
}

void HealthApp::drawGoalValue(GoalField field) {
    int16_t y = fieldY(field);

    // clear just the value area so the buttons don't get wiped
    _tft.fillRect(GOAL_VAL_X, y + 2, GOAL_VAL_W - 4, GOAL_ROW_H - 4, H_BAR_BG);

    _tft.setTextDatum(CL_DATUM);
    _tft.setTextColor(H_TEXT, H_BAR_BG);

    char buf[32];

    switch (field) {
        case GoalField::FIELD_CALORIES:
            snprintf(buf, sizeof(buf), "KCAL: %u", _goalCalories);
            break;
        case GoalField::FIELD_STEPS:
            snprintf(buf, sizeof(buf), "STEPS: %u", _goalSteps);
            break;
        case GoalField::FIELD_MINUTES:
            snprintf(buf, sizeof(buf), "ACT. MINS: %u", _goalMinutes);
            break;
    }

    _tft.drawString(buf, GOAL_VAL_X + 6, y + GOAL_ROW_H / 2, 2);
}

void HealthApp::drawFieldHighlight(GoalField field, bool selected) {
    int16_t  y   = fieldY(field);
    uint16_t col = selected ? H_SEL : H_MUTED;

    _tft.drawRect(GOAL_MINUS_X, y, 316, GOAL_ROW_H, col);
}

void HealthApp::drawProgressScreen() {
    _tft.fillScreen(H_BG);

    _tft.setTextDatum(TC_DATUM);
    _tft.setTextColor(H_TEXT, H_BG);
    _tft.drawString("MY PROGRESS", 160, 8, 4);

    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(H_MUTED, H_BG);
    _tft.drawString("CALORIES",   BAR_X, BAR_LABEL1_Y, 2);
    _tft.drawString("STEPS",      BAR_X, BAR_LABEL2_Y, 2);
    _tft.drawString("ACT. MINS",  BAR_X, BAR_LABEL3_Y, 2);

    drawBar(BAR_X, BAR1_Y, BAR_W, BAR_H, (uint32_t)_curCalories, _goalCalories, H_FILL_CAL);
    drawBar(BAR_X, BAR2_Y, BAR_W, BAR_H, _curSteps,              _goalSteps,    H_FILL_STP);
    drawBar(BAR_X, BAR3_Y, BAR_W, BAR_H, _curMinutes,            _goalMinutes,  H_FILL_MIN);

    _ui.drawRetroButton(BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H, 10, 5,
                        "BACK", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_BLACK);
}

void HealthApp::drawBar(int16_t x, int16_t y, int16_t w, int16_t h,
                        uint32_t current, uint32_t goal, uint16_t fillCol) {
    _tft.fillRoundRect(x, y, w, h, 5, H_BAR_BG);
    _tft.drawRoundRect(x, y, w, h, 5, H_MUTED);

    if (goal == 0) return;

    float ratio = (float)current / (float)goal;
    if (ratio > 1.0f) ratio = 1.0f;

    int16_t fillW = (int16_t)(ratio * (w - 4));

    if (fillW > 0) {
        _tft.fillRoundRect(x + 2, y + 2, fillW, h - 4, 4, fillCol);
    }

    char buf[12];
    snprintf(buf, sizeof(buf), "%u%%", (uint32_t)(ratio * 100));

    int16_t textX = x + 10;
    int16_t textY = y + h / 2;

    // dark shadow one pixel down/right so text pops on any background
    _tft.setTextDatum(ML_DATUM);
    _tft.setTextColor(TFT_BLACK, TFT_BLACK);
    _tft.drawString(buf, textX + 1, textY + 1, 2);

    // white text on top
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(buf, textX, textY, 2);
}

// ---- touch handlers ---------------------------------------------------------

void HealthApp::handleMenuTouch(uint16_t tx, uint16_t ty) {
    if (isHomeTouch(tx, ty)) {
        _wantsHome = true;
        return;
    }

    if (tx >= MENU_BTN_X && tx <= MENU_BTN_X + MENU_BTN_W &&
        ty >= MENU_SET_Y  && ty <= MENU_SET_Y  + MENU_BTN_H) {
        _screen = HealthScreen::HEALTH_GOALS;
        _field  = GoalField::FIELD_CALORIES;
        drawGoalsScreen();
        return;
    }

    if (tx >= MENU_BTN_X && tx <= MENU_BTN_X + MENU_BTN_W &&
        ty >= MENU_PROG_Y && ty <= MENU_PROG_Y + MENU_BTN_H) {
        _screen = HealthScreen::HEALTH_PROGRESS;
        drawProgressScreen();
        return;
    }
}

void HealthApp::handleGoalsTouch(uint16_t tx, uint16_t ty) {
    // save tapped -> write to NVS and go back to the menu
    if (tx >= SAVE_BTN_X && tx <= SAVE_BTN_X + SAVE_BTN_W &&
        ty >= SAVE_BTN_Y && ty <= SAVE_BTN_Y + SAVE_BTN_H) {
        saveGoals();
        _screen = HealthScreen::HEALTH_MENU;
        drawMenu();
        return;
    }

    // check which row was tapped
    for (int i = 0; i < 3; i++) {
        GoalField f = (GoalField)i;
        int16_t   y = fieldY(f);

        if (ty < (uint16_t)y || ty > (uint16_t)(y + GOAL_ROW_H)) continue;

        // switch selected row if needed
        if (_field != f) {
            GoalField prev = _field;
            _field = f;
            drawFieldHighlight(prev, false);
            drawFieldHighlight(f, true);
        }

        // minus
        if (tx >= (uint16_t)GOAL_MINUS_X && tx <= (uint16_t)(GOAL_MINUS_X + GOAL_SIDE_W)) {
            switch (f) {
                case GoalField::FIELD_CALORIES: if (_goalCalories > 50)   _goalCalories -= 50;  break;
                case GoalField::FIELD_STEPS:    if (_goalSteps    > 500)  _goalSteps    -= 500; break;
                case GoalField::FIELD_MINUTES:  if (_goalMinutes  > 5)    _goalMinutes  -= 5;   break;
            }
            drawGoalValue(f);
        }
        // plus
        else if (tx >= (uint16_t)GOAL_PLUS_X && tx <= (uint16_t)(GOAL_PLUS_X + GOAL_SIDE_W)) {
            switch (f) {
                case GoalField::FIELD_CALORIES: if (_goalCalories < 5000)  _goalCalories += 50;  break;
                case GoalField::FIELD_STEPS:    if (_goalSteps    < 50000) _goalSteps    += 500; break;
                case GoalField::FIELD_MINUTES:  if (_goalMinutes  < 300)   _goalMinutes  += 5;   break;
            }
            drawGoalValue(f);
        }

        break;
    }
}

void HealthApp::handleProgressTouch(uint16_t tx, uint16_t ty) {
    // back goes to the health menu, not all the way home
    if (tx >= (uint16_t)BACK_BTN_X && tx <= (uint16_t)(BACK_BTN_X + BACK_BTN_W) &&
        ty >= (uint16_t)BACK_BTN_Y && ty <= (uint16_t)(BACK_BTN_Y + BACK_BTN_H)) {
        _screen = HealthScreen::HEALTH_MENU;
        drawMenu();
    }
}

// ---- helpers ----------------------------------------------------------------

bool HealthApp::isHomeTouch(uint16_t tx, uint16_t ty) const {
    return tx >= (uint16_t)H_HOME_X && tx <= (uint16_t)(H_HOME_X + H_HOME_W) &&
           ty >= (uint16_t)H_HOME_Y && ty <= (uint16_t)(H_HOME_Y + H_HOME_H);
}

int16_t HealthApp::fieldY(GoalField field) const {
    switch (field) {
        case GoalField::FIELD_CALORIES: return GOAL_ROW1_Y;
        case GoalField::FIELD_STEPS:    return GOAL_ROW2_Y;
        case GoalField::FIELD_MINUTES:  return GOAL_ROW3_Y;
        default:                        return GOAL_ROW1_Y;
    }
}
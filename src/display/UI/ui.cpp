// Date parts commented out, not deleted. check lines 43, 108-110, 190-212
// UI.cpp
// See UI.h for layout description, geometry and colour constants.

#include "UI.h"

UI *UI::_instance = nullptr;

// button positions inside the right panel
static const int16_t BTN_COL1_X = RIGHT_PNL_X + 6;
static const int16_t BTN_COL2_X = RIGHT_PNL_X + 6 + BTN_W + BTN_GAP;
static const int16_t BTN_ROW1_Y = RIGHT_PNL_Y + 8;
static const int16_t BTN_ROW2_Y = RIGHT_PNL_Y + 8 + BTN_H + BTN_GAP;

static const char   *BTN_LABEL[4]  = { "C.T", "S.T", "S.C.T", "P.ID.T" };
static const bool    BTN_ACTIVE[4] = { true, false, true, false };
static const int16_t BTN_X[4] = { BTN_COL1_X, BTN_COL2_X, BTN_COL1_X, BTN_COL2_X };
static const int16_t BTN_Y[4] = { BTN_ROW1_Y, BTN_ROW1_Y, BTN_ROW2_Y, BTN_ROW2_Y };

// ---- constructor -----------------------------------------------------------

UI::UI(TFT_eSPI &tft, StepCounter &stepM, Calibration &cal)
    : _tft(tft), _stepM(stepM), _cal(cal) {
    _instance = this;
}

// ---- public ----------------------------------------------------------------

void UI::begin() {
    _gif.begin(GIF_PALETTE_RGB565_BE);
    _tft.fillScreen(GB_LIGHTEST);
    drawStaticLayout();
    drawStandingGif();
}

void UI::update(uint32_t nowMs, float cv, float cp) {
    uint32_t steps = _stepM.getStepCount();

    updateActivity();
    advanceSprite(nowMs);
    
    refreshTime();
    // refreshDate();
    refreshSteps(steps);
    refreshBPM();
    refreshBattery(cv, cp);
}

void UI::setTime(uint8_t hour, uint8_t minute) {
    _hour   = hour;
    _minute = minute;
}

void UI::setDate(uint8_t day, uint8_t month) {
    _day   = day;
    _month = month;
}

void UI::setBPM(int bpm) {
    _bpm = bpm;
}

void UI::setPace(const char *pace) {
    _pace = pace;
}

bool UI::checkButtonTouch(uint16_t tx, uint16_t ty, uint8_t &btnIndex) {
    for (uint8_t i = 0; i < 4; i++) {
        if (tx >= (uint16_t)BTN_X[i] && tx <= (uint16_t)(BTN_X[i] + BTN_W) &&
            ty >= (uint16_t)BTN_Y[i] && ty <= (uint16_t)(BTN_Y[i] + BTN_H)) {
            btnIndex = i;
            return true;
        }
    }

    return false;
}

// ---- static layout ---------------------------------------------------------

void UI::drawStaticLayout() {
    drawTopBar();
    drawRightPanel();
    drawLeftPanel();
    drawStepsBar();
}

void UI::drawTopBar() {
    _tft.fillRect(0, 0, SCREEN_W, TOPBAR_H, GB_LIGHTEST);
    _tft.drawFastHLine(0, TOPBAR_H, SCREEN_W, GB_DARK);

    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    _tft.drawString("--:--", 6, 3, 2);
}

void UI::drawStepsBar() {
    _tft.fillRect(STEPS_BAR_X, STEPS_BAR_Y, STEPS_BAR_W, STEPS_BAR_H, GB_LIGHT);
    _tft.drawRect(STEPS_BAR_X, STEPS_BAR_Y, STEPS_BAR_W, STEPS_BAR_H, GB_DARKEST);

    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARK, GB_LIGHT);
    _tft.drawString("STEPS", STEPS_BAR_X + 8, STEPS_BAR_Y + 6, 2);

    _tft.setTextColor(GB_DARKEST, GB_LIGHT);
    _tft.drawString("0", STEPS_BAR_X + 8, STEPS_BAR_Y + 28, 4);

    // _tft.setTextDatum(TR_DATUM);
    // _tft.drawString("--/--", STEPS_BAR_X + STEPS_BAR_W - 8,
                    // STEPS_BAR_Y + STEPS_BAR_H - 22, 2);
}

void UI::drawLeftPanel() {
    _tft.fillRect(LEFT_PNL_X, LEFT_PNL_Y, LEFT_PNL_W, LEFT_PNL_H, GB_LIGHT);
    _tft.drawRect(LEFT_PNL_X, LEFT_PNL_Y, LEFT_PNL_W, LEFT_PNL_H, GB_DARKEST);

    _tft.drawFastHLine(LEFT_PNL_X + 3,
                       LEFT_PNL_Y + LEFT_PNL_H / 2,
                       LEFT_PNL_W - 6,
                       GB_DARK);

    drawHeartIcon(LEFT_PNL_X + 17, LEFT_PNL_Y + 20);

    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);
    _tft.drawString("-- BPM", LEFT_PNL_X + 34, LEFT_PNL_Y + 12, 2);

    drawBattIcon(LEFT_PNL_X + 12, LEFT_PNL_Y + 58, 0);
    _tft.drawString("--V", LEFT_PNL_X + 42, LEFT_PNL_Y + 54, 2);
}

void UI::drawRightPanel() {
    _tft.fillRect(RIGHT_PNL_X, RIGHT_PNL_Y, RIGHT_PNL_W, RIGHT_PNL_H, GB_LIGHT);
    _tft.drawRect(RIGHT_PNL_X, RIGHT_PNL_Y, RIGHT_PNL_W, RIGHT_PNL_H, GB_DARKEST);

    for (uint8_t i = 0; i < 4; i++) {
        drawButton(BTN_X[i], BTN_Y[i], BTN_LABEL[i], BTN_ACTIVE[i]);
    }
}

void UI::drawButton(int16_t x, int16_t y, const char *label, bool active) {
    uint16_t bg  = active ? GB_LIGHT   : GB_INACTIVE;
    uint16_t bor = active ? GB_DARKEST : GB_DARK;
    uint16_t txt = active ? GB_DARKEST : GB_DARK;

    _tft.fillRect(x, y, BTN_W, BTN_H, bg);
    _tft.drawRect(x, y, BTN_W, BTN_H, bor);

    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(txt, bg);
    _tft.drawString(label, x + BTN_W / 2, y + BTN_H / 2, 2);
}

void UI::drawHeartIcon(int16_t cx, int16_t cy) {
    _tft.fillCircle(cx - 3, cy - 2, 4, HEART_RED);
    _tft.fillCircle(cx + 3, cy - 2, 4, HEART_RED);
    _tft.fillTriangle(cx - 7, cy, cx + 7, cy, cx, cy + 7, HEART_RED);
}

void UI::drawBattIcon(int16_t x, int16_t y, int8_t pct) {
    _tft.fillRect(x, y, 18, 10, GB_LIGHTEST);
    _tft.drawRect(x, y, 18, 10, GB_DARKEST);
    _tft.fillRect(x + 18, y + 3, 3, 4, GB_DARKEST);

    if (pct < 0) return;

    uint8_t fillW = (uint8_t)((pct / 100.0f) * 14);
    if (fillW > 0) {
        _tft.fillRect(x + 2, y + 2, fillW, 6, (pct > 20) ? GB_MID : TFT_RED);
    }
}

// ---- region refreshers -----------------------------------------------------

void UI::refreshTime() {
    if (_hour == _lastHour && _minute == _lastMinute) return;

    _lastHour   = _hour;
    _lastMinute = _minute;

    _tft.fillRect(4, 2, 80, TOPBAR_H - 2, GB_LIGHTEST);
    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHTEST);

    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", _hour, _minute);
    _tft.drawString(buf, 6, 3, 2);
}

// void UI::refreshDate() {
//     if (_day == _lastDay && _month == _lastMonth) return;

//     _lastDay   = _day;
//     _lastMonth = _month;

//     _tft.fillRect(STEPS_BAR_X + STEPS_BAR_W - 70,
//                   STEPS_BAR_Y + STEPS_BAR_H - 24,
//                   64,
//                   20,
//                   GB_LIGHT);

//     _tft.setTextDatum(TR_DATUM);
//     _tft.setTextColor(GB_DARKEST, GB_LIGHT);

//     char buf[6];
//     snprintf(buf, sizeof(buf), "%02d/%02d", _day, _month);

//     _tft.drawString(buf,
//                     STEPS_BAR_X + STEPS_BAR_W - 8,
//                     STEPS_BAR_Y + STEPS_BAR_H - 22,
//                     2);
// }

void UI::refreshSteps(uint32_t steps) {
    if (steps == _lastSteps) return;

    _lastSteps = steps;

    _tft.fillRect(STEPS_BAR_X + 6,
                  STEPS_BAR_Y + 26,
                  STEPS_BAR_W - 12,
                  34,
                  GB_LIGHT);

    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);

    char buf[10];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)steps);

    _tft.drawString(buf, STEPS_BAR_X + 8, STEPS_BAR_Y + 28, 4);
}

void UI::refreshBPM() {
    if (_bpm == _lastBPM) return;

    _lastBPM = _bpm;

    _tft.fillRect(LEFT_PNL_X + 32,
                  LEFT_PNL_Y + 8,
                  LEFT_PNL_W - 38,
                  28,
                  GB_LIGHT);

    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);

    char buf[10];

    if (_bpm < 0) {
        snprintf(buf, sizeof(buf), "-- BPM");
    } else {
        snprintf(buf, sizeof(buf), "%d BPM", _bpm);
    }

    _tft.drawString(buf, LEFT_PNL_X + 34, LEFT_PNL_Y + 12, 2);
}

void UI::refreshBattery(float cv, float cp) {
    int8_t pct = (cp >= 0) ? (int8_t)cp : -1;
    uint16_t battMv = (uint16_t)(cv * 1000.0f);

    // Avoid unnecessary redraw
    if (pct == _lastBattPct && battMv == _lastBattMv) return;

    _lastBattPct = pct;
    _lastBattMv  = battMv;

    _tft.fillRect(LEFT_PNL_X + 4,
                  LEFT_PNL_Y + 47,
                  LEFT_PNL_W - 8,
                  30,
                  GB_LIGHT);

    // Draw icon (fallback to 0% if invalid)
    drawBattIcon(LEFT_PNL_X + 12, LEFT_PNL_Y + 58,
                 (pct >= 0) ? pct : 0);

    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);

    char buf[12];

    if (pct < 0) {
        // If percentage invalid → show voltage
        snprintf(buf, sizeof(buf), "%.2fV", cv);
    } else {
        snprintf(buf, sizeof(buf), "%d%%", pct);
    }

    _tft.drawString(buf, LEFT_PNL_X + 42, LEFT_PNL_Y + 54, 2);
}
// ---- sprite / activity -----------------------------------------------------

void UI::updateActivity() {
    UIActivity newActivity;

    if (strcmp(_pace, "RUNNING") == 0) {
        newActivity = UIActivity::RUNNING;
    } else if (strcmp(_pace, "WALKING") == 0) {
        newActivity = UIActivity::WALKING;
    } else {
        newActivity = UIActivity::STANDING;
    }

    if (newActivity != _activity) {
        _activity     = newActivity;
        _gifFrame     = 0;
        _lastFrameMs  = 0;
        _lastActivity = UIActivity::NONE;

        _tft.fillRect(SPRITE_X, SPRITE_Y, SPRITE_W, SPRITE_H, GB_LIGHTEST);
    }
}

void UI::advanceSprite(uint32_t nowMs) {
    if (_activity == _lastActivity && _activity != UIActivity::NONE) {
        if (_activity == UIActivity::RUNNING) {
            drawGifFrame(run_gif, run_gif_len, UI_RUN_FRAME_MS, nowMs);
        } else if (_activity == UIActivity::WALKING) {
            drawGifFrame(walk_gif, walk_gif_len, UI_WALK_FRAME_MS, nowMs);
        }
        return;
    }

    _tft.fillRect(SPRITE_X, SPRITE_Y, SPRITE_W, SPRITE_H, GB_LIGHTEST);

    switch (_activity) {
        case UIActivity::STANDING:
            drawStandingGif();
            break;

        case UIActivity::RUNNING:
            drawGifFrame(run_gif, run_gif_len, UI_RUN_FRAME_MS, nowMs);
            break;

        case UIActivity::WALKING:
            drawGifFrame(walk_gif, walk_gif_len, UI_WALK_FRAME_MS, nowMs);
            break;

        case UIActivity::NONE:
            drawStandingGif();
            break;
    }

    _lastActivity = _activity;
}

void UI::drawStandingGif() {
    _tft.fillRect(SPRITE_X, SPRITE_Y, SPRITE_W, SPRITE_H, GB_LIGHTEST);

    int frameCount = _gif.openFLASH((uint8_t *)stand_gif, stand_gif_len, gifDraw);
    if (frameCount > 0) {
        _gif.playFrame(false, nullptr);
        _gif.close();
    }
}

void UI::drawGifFrame(const uint8_t *data, size_t len,
                      uint32_t frameMs, uint32_t nowMs) {
    if (nowMs - _lastFrameMs < frameMs) return;

    _lastFrameMs = nowMs;

    _tft.fillRect(SPRITE_X, SPRITE_Y, SPRITE_W, SPRITE_H, GB_LIGHTEST);

    int frameCount = _gif.openFLASH((uint8_t *)data, len, gifDraw);
    if (frameCount <= 0) return;

    for (int i = 0; i < _gifFrame; i++) {
        if (!_gif.playFrame(false, nullptr)) break;
    }

    _gif.playFrame(false, nullptr);
    _gif.close();

    _gifFrame = (_gifFrame + 1) % frameCount;
}

// ---- library callbacks -----------------------------------------------------

void UI::gifDraw(GIFDRAW *pDraw) {
    if (!_instance) return;

    UI &self = *_instance;

    int16_t y  = SPRITE_Y + pDraw->iY + pDraw->y;
    int16_t x0 = SPRITE_X + pDraw->iX;

    if (pDraw->ucHasTransparency) {
        uint8_t  *src     = pDraw->pPixels;
        uint16_t *palette = pDraw->pPalette;
        uint8_t   trans   = pDraw->ucTransparent;

        for (int i = 0; i < pDraw->iWidth; i++) {
            if (src[i] != trans) {
                self._tft.drawPixel(x0 + i, y, palette[src[i]]);
            }
        }
    } else {
        uint16_t *palette = pDraw->pPalette;

        for (int i = 0; i < pDraw->iWidth; i++) {
            uint8_t idx = pDraw->pPixels[i];

            if (palette[idx] != 0x0000) {
                self._tft.drawPixel(x0 + i, y, palette[idx]);
            }
        }
    }
}

// ---- battery ---------------------------------------------------------------

float UI::readBatteryVoltage() {
    uint32_t sum = 0;

    for (uint8_t i = 0; i < 8; i++) {
        sum += analogRead(BATT_ADC_PIN);
    }

    float vAdc  = (sum / 8.0f / 4095.0f) * 3.3f;
    float vBatt = vAdc * BATT_DIVIDER_RATIO;

    return vBatt;
}

int8_t UI::readBatteryPercent(float vBatt) {
    if (vBatt >= BATT_USB_THRESHOLD) return -1;

    float clamped = constrain(vBatt, BATT_MIN_V, BATT_MAX_V);

    return (int8_t)(((clamped - BATT_MIN_V) / (BATT_MAX_V - BATT_MIN_V)) * 100.0f);
}
// UI.cpp
// See UI.h for layout description, geometry and colour constants.
 
#include "UI.h"
 
// static instance pointer for GIF/PNG callbacks
UI *UI::_instance = nullptr;
 
// button positions inside the right panel (2x2 grid)
static const int16_t BTN_COL1_X = RIGHT_PNL_X + 4;
static const int16_t BTN_COL2_X = RIGHT_PNL_X + 4 + BTN_W + BTN_GAP;
static const int16_t BTN_ROW1_Y = BOTTOM_Y + 4;
static const int16_t BTN_ROW2_Y = BOTTOM_Y + 4 + BTN_H + BTN_GAP;
 
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
    drawStandingGif();        // initial display
}
 
void UI::update(uint32_t nowMs) {
    uint32_t steps = _stepM.getStepCount();
 
    updateActivity();
    advanceSprite(nowMs);
 
    refreshTime();
    refreshDate();
    refreshSteps(steps);
    refreshBPM();
    refreshBattery();
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
    drawStepsBar();
    drawLeftPanel();
    drawRightPanel();
}
 
void UI::drawTopBar() {
    _tft.fillRect(0, 0, 240, TOPBAR_H, GB_LIGHTEST);
    _tft.drawFastHLine(0, TOPBAR_H, 240, GB_DARK);
    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    _tft.drawString("--:--", 6, 3, 2);
}
 
void UI::drawStepsBar() {
    _tft.fillRect(STEPS_BAR_X, STEPS_BAR_Y, STEPS_BAR_W, STEPS_BAR_H, GB_LIGHT);
    _tft.drawRect(STEPS_BAR_X, STEPS_BAR_Y, STEPS_BAR_W, STEPS_BAR_H, GB_DARKEST);
 
    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARK, GB_LIGHT);
    _tft.drawString("STEPS", STEPS_BAR_X + 8, STEPS_BAR_Y + 4, 1);
 
    _tft.drawFastVLine(148, STEPS_BAR_Y + 2, STEPS_BAR_H - 4, GB_DARK);
 
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);
    _tft.drawString("0", STEPS_BAR_X + 8, STEPS_BAR_Y + 20, 4);
    _tft.setTextDatum(TR_DATUM);
    _tft.drawString("--/--", STEPS_BAR_X + STEPS_BAR_W - 4, STEPS_BAR_Y + 24, 2);
}
 
void UI::drawLeftPanel() {
    _tft.fillRect(LEFT_PNL_X, BOTTOM_Y, LEFT_PNL_W, BOTTOM_H, GB_LIGHT);
    _tft.drawRect(LEFT_PNL_X, BOTTOM_Y, LEFT_PNL_W, BOTTOM_H, GB_DARKEST);
 
    _tft.drawFastHLine(LEFT_PNL_X + 2, BOTTOM_Y + BOTTOM_H / 2,
                       LEFT_PNL_W - 4, GB_DARK);
 
    drawHeartIcon(LEFT_PNL_X + 14, BOTTOM_Y + 18);
    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);
    _tft.drawString("-- BPM", LEFT_PNL_X + 28, BOTTOM_Y + 10, 2);
 
    drawBattIcon(LEFT_PNL_X + 8, BOTTOM_Y + 48, 0);
    _tft.drawString("--%", LEFT_PNL_X + 34, BOTTOM_Y + 48, 2);
}
 
void UI::drawRightPanel() {
    _tft.fillRect(RIGHT_PNL_X, BOTTOM_Y, RIGHT_PNL_W, BOTTOM_H, GB_LIGHT);
    _tft.drawRect(RIGHT_PNL_X, BOTTOM_Y, RIGHT_PNL_W, BOTTOM_H, GB_DARKEST);
 
    for (uint8_t i = 0; i < 4; i++) {
        drawButton(BTN_X[i], BTN_Y[i], BTN_LABEL[i], BTN_ACTIVE[i]);
    }
}
 
void UI::drawButton(int16_t x, int16_t y, const char *label, bool active) {
    uint16_t bg  = active ? GB_LIGHT     : GB_INACTIVE;
    uint16_t bor = active ? GB_DARKEST : GB_DARK;
    uint16_t txt = active ? GB_DARKEST : GB_DARK;
 
    _tft.fillRect(x, y, BTN_W, BTN_H, bg);
    _tft.drawRect(x, y, BTN_W, BTN_H, bor);
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(txt, bg);
    _tft.drawString(label, x + BTN_W / 2, y + BTN_H / 2, 1);
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
 
 
// ---- region refreshers (all dirty-tracked) ---------------------------------
 
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
 
void UI::refreshDate() {
    if (_day == _lastDay && _month == _lastMonth) return;
    _lastDay   = _day;
    _lastMonth = _month;
 
    _tft.fillRect(150, STEPS_BAR_Y + 22, 82, 18, GB_LIGHT);
    _tft.setTextDatum(TR_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);
 
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d/%02d", _day, _month);
    _tft.drawString(buf, STEPS_BAR_X + STEPS_BAR_W - 4, STEPS_BAR_Y + 24, 2);
}
 
void UI::refreshSteps(uint32_t steps) {
    if (steps == _lastSteps) return;
    _lastSteps = steps;
 
    _tft.fillRect(STEPS_BAR_X + 2, STEPS_BAR_Y + 18, 142, 22, GB_LIGHT);
    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);
 
    char buf[10];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)steps);
    _tft.drawString(buf, STEPS_BAR_X + 8, STEPS_BAR_Y + 20, 4);
}
 
void UI::refreshBPM() {
    if (_bpm == _lastBPM) return;
    _lastBPM = _bpm;
 
    _tft.fillRect(LEFT_PNL_X + 26, BOTTOM_Y + 6, LEFT_PNL_W - 30, 22, GB_LIGHT);
    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);
 
    char buf[10];
    if (_bpm < 0) {
        snprintf(buf, sizeof(buf), "-- BPM");
    } else {
        snprintf(buf, sizeof(buf), "%d BPM", _bpm);
    }
    _tft.drawString(buf, LEFT_PNL_X + 28, BOTTOM_Y + 10, 2);
}
 
void UI::refreshBattery() {
    int8_t pct = readBatteryPercent();
    if (pct == _lastBattPct) return;
    _lastBattPct = pct;
 
    _tft.fillRect(LEFT_PNL_X + 2, BOTTOM_Y + 44, LEFT_PNL_W - 4, 30, GB_LIGHT);
 
    drawBattIcon(LEFT_PNL_X + 8, BOTTOM_Y + 48, pct);
 
    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(GB_DARKEST, GB_LIGHT);
 
    char buf[8];
    if (pct < 0) {
        snprintf(buf, sizeof(buf), "AC");
    } else {
        snprintf(buf, sizeof(buf), "%d%%", pct);
    }
    _tft.drawString(buf, LEFT_PNL_X + 34, BOTTOM_Y + 48, 2);
}
 
// ---- sprite / activity -----------------------------------------------------
 
void UI::updateActivity() {
    // map the pace string from pacefind straight to the activity enum
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
        _lastActivity = UIActivity::NONE;  // force a redraw
        _tft.fillRect(SPRITE_X, SPRITE_Y, SPRITE_W, SPRITE_H, GB_LIGHT);
    }
}
 
void UI::advanceSprite(uint32_t nowMs) {
    // if activity hasn't changed and it's not the first time, just continue animation
    if (_activity == _lastActivity && _activity != UIActivity::NONE) {
        if (_activity == UIActivity::RUNNING) {
            drawGifFrame(run_gif, run_gif_len, UI_RUN_FRAME_MS, nowMs);
        } else if (_activity == UIActivity::WALKING) {
            drawGifFrame(walk_gif, walk_gif_len, UI_WALK_FRAME_MS, nowMs);
        }
        return;
    }
 
    // first time or activity changed - draw the appropriate animation
    _tft.fillRect(SPRITE_X, SPRITE_Y, SPRITE_W, SPRITE_H, GB_LIGHT);
 
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
 
        case UIActivity::NONE:  // fallback on first boot
            drawStandingGif();
            break;
    }
 
    _lastActivity = _activity;
}
 
void UI::drawStandingGif() {
    _tft.fillRect(SPRITE_X, SPRITE_Y, SPRITE_W, SPRITE_H, GB_LIGHT);
 
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
 
    _tft.fillRect(SPRITE_X, SPRITE_Y, SPRITE_W, SPRITE_H, GB_LIGHT);
 
    int frameCount = _gif.openFLASH((uint8_t *)data, len, gifDraw);
    if (frameCount <= 0) return;
 
    // seek to current frame
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
 
    int16_t y = SPRITE_Y + pDraw->iY + pDraw->y;
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
            if (palette[idx] != 0x0000) {  // skip pure black
                self._tft.drawPixel(x0 + i, y, palette[idx]);
            }
        }
    }
}
 
// ---- battery ---------------------------------------------------------------
 
int8_t UI::readBatteryPercent() {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 8; i++) sum += analogRead(BATT_ADC_PIN);
    float vAdc  = (sum / 8.0f / 4095.0f) * 3.3f;
    float vBatt = vAdc * BATT_DIVIDER_RATIO;
    if (vBatt >= BATT_USB_THRESHOLD) return -1;
    float clamped = constrain(vBatt, BATT_MIN_V, BATT_MAX_V);
    return (int8_t)(((clamped - BATT_MIN_V) / (BATT_MAX_V - BATT_MIN_V)) * 100.0f);
}
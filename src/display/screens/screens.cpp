/* screens.cpp
 Implements the static screen helpers declared in screens.h. */

#include "screens.h"
#include "../UI/UI.h"
#include <cmath>

static constexpr int16_t HOME_BTN_X = 95;
static constexpr int16_t HOME_BTN_Y = 184;
static constexpr int16_t HOME_BTN_W = 130;
static constexpr int16_t HOME_BTN_H = 48;
static constexpr int16_t HOME_BTN_CR = 10;  // Corner radius of home button

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

// Local retro button helper used by setup and calibration screens.
// This keeps the same visual style as the home/app buttons without changing hitboxes.
static void drawLocalRetroButton(TFT_eSPI &tft,
                                 int16_t x, int16_t y,
                                 int16_t w, int16_t h,
                                 int16_t cornerRadius,
                                 const char *label,
                                 uint8_t fontSize = 4) {
    static constexpr int16_t SHADOW_OFFSET = 4;
    static constexpr int16_t INNER_OFFSET  = 3;

    tft.fillRoundRect(x, y, w, h, cornerRadius, BTN_SHADOW);
    tft.fillRoundRect(x, y, w, h - SHADOW_OFFSET, cornerRadius, BTN_GLARE);
    tft.fillRoundRect(x + INNER_OFFSET,
                      y + INNER_OFFSET,
                      w - INNER_OFFSET,
                      h - SHADOW_OFFSET - INNER_OFFSET,
                      cornerRadius,
                      GB_BUTTON);

    tft.drawRoundRect(x, y, w, h, cornerRadius, TFT_BLACK);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, GB_BUTTON);
    tft.drawString(label, x + w / 2, y + h / 2 - 1, fontSize);
    tft.setTextDatum(TL_DATUM);
}

static void drawHomeButton(TFT_eSPI &tft, UI &ui) {
    ui.drawRetroButton(HOME_BTN_X, HOME_BTN_Y, HOME_BTN_W, HOME_BTN_H, HOME_BTN_CR, 6, "HOME", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
}

static void drawSetupButtons(TFT_eSPI &tft) {
    drawLocalRetroButton(tft, MINUS_BTN_X, SETUP_BTN_Y, SIDE_BTN_W, SETUP_BTN_H, 6, "-", 4);
    drawLocalRetroButton(tft, OK_BTN_X,    SETUP_BTN_Y, OK_BTN_W,   SETUP_BTN_H, 6, "OK", 4);
    drawLocalRetroButton(tft, PLUS_BTN_X,  SETUP_BTN_Y, SIDE_BTN_W, SETUP_BTN_H, 6, "+", 4);
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

// draws a single bold arrow centred at (cx, cy)
// dir: 0=right, 1=left, 2=up, 3=down, 4=diag up-right, 5=diag down-left
// size controls how big it is
static void drawArrow(TFT_eSPI &tft, int cx, int cy, int dir, int size, uint16_t color) {
    int shaft  = size / 2;
    int head   = size / 3;
    int spread = size / 4;
    int thick  = 4;

    if (dir == 4 || dir == 5) {
        int diag = (int)(shaft * 0.707f);

        int tx = cx + diag, ty = cy - diag;
        int bx = cx - diag, by = cy + diag;

        if (dir == 5) {
            tx = cx - diag; ty = cy + diag;
            bx = cx + diag; by = cy - diag;
        }

        for (int t = -thick / 2; t <= thick / 2; t++) {
            tft.drawLine(bx + t, by - t, tx + t, ty - t, color);
            tft.drawLine(bx - t, by + t, tx - t, ty + t, color);
        }

        if (dir == 4) {
            tft.fillTriangle(tx, ty,
                             tx - head, ty,
                             tx, ty + head,
                             color);
        } else {
            tft.fillTriangle(tx, ty,
                             tx + head, ty,
                             tx, ty - head,
                             color);
        }
        return;
    }

    int tx, ty, bx, by;

    if (dir == 0) {
        tx = cx + shaft; ty = cy;
        bx = cx - shaft; by = cy;
    } else if (dir == 1) {
        tx = cx - shaft; ty = cy;
        bx = cx + shaft; by = cy;
    } else if (dir == 2) {
        tx = cx; ty = cy - shaft;
        bx = cx; by = cy + shaft;
    } else {
        tx = cx; ty = cy + shaft;
        bx = cx; by = cy - shaft;
    }

    if (dir == 0 || dir == 1) {
        int x1 = min(tx, bx);
        tft.fillRect(x1, cy - thick / 2, abs(tx - bx), thick, color);
    } else {
        int y1 = min(ty, by);
        tft.fillRect(cx - thick / 2, y1, thick, abs(ty - by), color);
    }

    if (dir == 0) {
        tft.fillTriangle(tx, ty,
                         tx - head, ty - spread,
                         tx - head, ty + spread,
                         color);
    } else if (dir == 1) {
        tft.fillTriangle(tx, ty,
                         tx + head, ty - spread,
                         tx + head, ty + spread,
                         color);
    } else if (dir == 2) {
        tft.fillTriangle(tx, ty,
                         tx - spread, ty + head,
                         tx + spread, ty + head,
                         color);
    } else {
        tft.fillTriangle(tx, ty,
                         tx - spread, ty - head,
                         tx + spread, ty - head,
                         color);
    }
}

// returns 0=right, 1=left, 2=up, 3=down based on which direction
// the watch needs to be tilted for each calibration step
// matches DIR_LABEL order: X+, X-, Y+, Y-, Z+, Z-
static int arrowDir(int dirIndex) {
    switch (dirIndex) {
        case 0: return 2;  // X+ UP is an upwards arrow
        case 1: return 3;  // X- UP is a downwards arrow
        case 2: return 0;  // Y+ UP is an arrow pointing to the right
        case 3: return 1;  // Y- UP is an arrow pointing to the left
        case 4: return 4;  // Z+ UP is an arrow pointing into the watch screen
        case 5: return 5;  // Z- UP is an arrow pointing out of the watch screen
        default: return 2;
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

    int arrowColor = isSampling ? TFT_GREEN : TFT_ORANGE;
    drawArrow(tft, 245, 120, arrowDir(dirIndex), 90, arrowColor);
}

void drawCalibrationDone(TFT_eSPI &tft, UI &ui, bool isReentry, uint32_t savedSteps) {
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

        drawLocalRetroButton(tft, 35, 184, 115, 48, 6, "CANCEL", 4);
        drawLocalRetroButton(tft, 170, 184, 115, 48, 6, "USE", 4);

    } else {
        drawHomeButton(tft, ui);
    }
}

void drawSWTime(TFT_eSPI &tft, const Stopwatch::SW_Time &t) {
    tft.fillRect(30, SW_TIME_Y, SW_TIME_W, SW_TIME_H, GB_LIGHTEST);

    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d.%02d",
             t._minutes, t._seconds, t._milliseconds);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, GB_LIGHTEST);
    tft.drawString(buf, SW_X, SW_TIME_Y + SW_TIME_H / 2, 4);
}

void eraseSWDot(TFT_eSPI &tft, Stopwatch &sw) {
    auto pos = sw.getCirclePosition();

    int r2outer = SW_OUTER_RADIUS * SW_OUTER_RADIUS;
    int r2inner = (SW_OUTER_RADIUS - SW_THICKNESS) * (SW_OUTER_RADIUS - SW_THICKNESS);

    tft.startWrite();
    for (int dy = -(SW_POINT_R + 1); dy <= (SW_POINT_R + 1); dy++) {
        for (int dx = -(SW_POINT_R + 1); dx <= (SW_POINT_R + 1); dx++) {
            int sx = pos.first  + dx;
            int sy = pos.second + dy;
            int d2 = (sx - SW_X)*(sx - SW_X) + (sy - SW_Y)*(sy - SW_Y);

            uint16_t col;
            if (d2 > r2outer)      col = GB_LIGHTEST;
            else if (d2 < r2inner) col = GB_LIGHTEST;
            else                   col = TFT_BLACK;

            tft.drawPixel(sx, sy, col);
        }
    }
    tft.endWrite();
}

void drawSWDot(TFT_eSPI &tft, Stopwatch &sw) {
    auto pos = sw.getCirclePosition();
    tft.fillCircle(pos.first, pos.second, SW_POINT_R, GB_BUTTON);
    tft.fillCircle(pos.first, pos.second, SW_POINT_R - 5, GB_LIGHTEST);
}

void drawSWScreen(TFT_eSPI &tft, Stopwatch &sw, UI &ui) {
    tft.fillScreen(GB_LIGHTEST);

    tft.setTextDatum(TL_DATUM);

    if (sw.getState() == Stopwatch::RUNNING) {
        ui.drawRetroButton(SW_BTN_X - SW_BTN_R, START_Y - SW_BTN_R, 2 * SW_BTN_R, 2 * SW_BTN_R, 6, 6, "STOP", 2,
                            RESET_RED, TFT_BLACK, RESET_SHADOW, RESET_GLARE, TFT_WHITE);
    } else {
        ui.drawRetroButton(SW_BTN_X - SW_BTN_R, START_Y - SW_BTN_R, 2 * SW_BTN_R, 2 * SW_BTN_R, 6, 6, "START", 2,
                            GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
    }

    ui.drawRetroButton(SW_BTN_X - SW_BTN_R, RESET_Y - SW_BTN_R, 2 * SW_BTN_R, 2 * SW_BTN_R, 6, 6, "RESET", 2,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);

    ui.drawRetroButton(SW_BTN_X - SW_BTN_R, SW_HOME_Y - SW_BTN_R, 2 * SW_BTN_R, 2 * SW_BTN_R, 6, 6, "HOME", 2,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);

    tft.fillCircle(SW_X, SW_Y, SW_OUTER_RADIUS, TFT_BLACK);
    tft.fillCircle(SW_X, SW_X, (SW_OUTER_RADIUS - SW_THICKNESS), GB_LIGHTEST);

    auto pos = sw.getCirclePosition();
    tft.fillCircle(pos.first, pos.second, SW_POINT_R, GB_BUTTON);
    tft.fillCircle(pos.first, pos.second, SW_POINT_R - 5, GB_LIGHTEST);

    drawSWTime(tft, sw.getFormattedTime());
}

void updateSWScreen(TFT_eSPI &tft, Stopwatch &sw) {
    auto prev = sw.getPrevCirclePosition();
    auto curr = sw.getCirclePosition();

    int r2outer = SW_OUTER_RADIUS * SW_OUTER_RADIUS;
    int r2inner = (SW_OUTER_RADIUS - SW_THICKNESS) * (SW_OUTER_RADIUS - SW_THICKNESS);
    int r2dot   = SW_POINT_R * SW_POINT_R;

    tft.startWrite();

    for (int dy = -(SW_POINT_R + 1); dy <= (SW_POINT_R + 1); dy++) {
        for (int dx = -(SW_POINT_R + 1); dx <= (SW_POINT_R + 1); dx++) {
            int sx = prev.first  + dx;
            int sy = prev.second + dy;

            int ndx = sx - curr.first;
            int ndy = sy - curr.second;
            if (ndx * ndx + ndy * ndy <= r2dot) continue;

            int d2 = (sx - SW_X)*(sx - SW_X) + (sy - SW_Y)*(sy - SW_Y);
            uint16_t col;
            if (d2 > r2outer)      col = GB_LIGHTEST;
            else if (d2 < r2inner) col = GB_LIGHTEST;
            else                   col = TFT_BLACK;

            tft.drawPixel(sx, sy, col);
        }
    }

    tft.endWrite();

    tft.fillCircle(curr.first, curr.second, SW_POINT_R, GB_BUTTON);
    tft.fillCircle(curr.first, curr.second, SW_POINT_R - 5, GB_LIGHTEST);

    drawSWTime(tft, sw.getFormattedTime());
}

void drawSelfTestScreen(TFT_eSPI &tft, UI &ui, bool passed, const char *resultStr,
                        float dX, float dY, float dZ) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString("SELF TEST", 10, 10, 4);

    tft.setTextColor(passed ? TFT_GREEN : TFT_RED, APP_BG);
    tft.drawString(resultStr, 10, 50, 4);

    tft.setTextColor(APP_MUTED, APP_BG);

    char buf[32];

    snprintf(buf, sizeof(buf), "dX = %.2f mV", dX);
    tft.drawString(buf, 10, 92, 4);

    snprintf(buf, sizeof(buf), "dY = %.2f mV", dY);
    tft.drawString(buf, 10, 126, 4);

    snprintf(buf, sizeof(buf), "dZ = %.2f mV", dZ);
    tft.drawString(buf, 10, 160, 4);

    drawHomeButton(tft, ui);
}

void drawStubScreen(TFT_eSPI &tft, UI &ui, const char *title) {
    tft.fillScreen(APP_BG);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(APP_TEXT, APP_BG);
    tft.drawString(title, 10, 10, 4);

    tft.setTextColor(APP_MUTED, APP_BG);
    tft.drawString("Not available", 10, 58, 4);

    drawHomeButton(tft, ui);
}

void drawGameScreen(TFT_eSPI &tft, UI &ui) {
    tft.fillScreen(GB_LIGHTEST);

    tft.drawRoundRect(GAME_X, GAME_Y, GAME_SIZE, GAME_SIZE, 10, TFT_BLACK);

    tft.drawFastHLine(20, 100, GAME_SIZE, TFT_BLACK);
    tft.drawFastHLine(20, 160, GAME_SIZE, TFT_BLACK);
    tft.drawFastVLine(80, 40, GAME_SIZE, TFT_BLACK);
    tft.drawFastVLine(140, 40, GAME_SIZE, TFT_BLACK);

    drawGamePlay(tft, ui);
    drawGameMode(tft, ui);
    drawGameHome(tft, ui);

    tft.setTextDatum(CC_DATUM);
    tft.setTextColor(TFT_BLACK);
    tft.drawString("TIC TAC TOE", 160, 20, 4);
    tft.setTextDatum(TL_DATUM);
    tft.drawFastHLine(80, 32, 160, TFT_BLACK);
}

std::pair<int, int> getCellXY(int row, int col) {
    std::pair<int, int> cell_coords;
    cell_coords.first = CELL_COORDS[row - 1][col - 1].first;
    cell_coords.second = CELL_COORDS[row - 1][col - 1].second;
    return cell_coords;
}

void drawGameX(TFT_eSPI &tft, int row, int col) {
    int cell_x = getCellXY(row, col).first;
    int cell_y = getCellXY(row, col).second;

    float shift_f = CROSS_LENGTH / (2 * sqrt(2));
    int shift = static_cast<int>(shift_f);

    int start1_x = cell_x - shift;
    int start1_y = cell_y - shift;
    int end1_x = cell_x + shift;
    int end1_y = cell_y + shift;
    tft.drawWideLine(start1_x, start1_y, end1_x, end1_y, 2, TFT_BLACK, GB_LIGHTEST);

    int start2_x = cell_x - shift;
    int start2_y = cell_y + shift;
    int end2_x = cell_x + shift;
    int end2_y = cell_y - shift;
    tft.drawWideLine(start2_x, start2_y, end2_x, end2_y, 2, TFT_BLACK, GB_LIGHTEST);
}

void drawGameO(TFT_eSPI &tft, int row, int col) {
    int cell_x = getCellXY(row, col).first;
    int cell_y = getCellXY(row, col).second;

    tft.fillSmoothCircle(cell_x, cell_y, CIRCLE_RADIUS, TFT_BLACK, GB_LIGHTEST);
    tft.fillSmoothCircle(cell_x, cell_y, CIRCLE_RADIUS - 2, GB_LIGHTEST, GB_LIGHTEST);
}

void drawGamePlay(TFT_eSPI &tft, UI &ui) {
    ui.drawRetroButton(212, 40, 96, 54, 8, 8, "PLAY", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
}

void drawGameMode(TFT_eSPI &tft, UI &ui) {
    ui.drawRetroButton(212, 103, 96, 54, 8, 8, "MODE", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
}

void drawGameHome(TFT_eSPI &tft, UI &ui) {
    ui.drawRetroButton(212, 166, 96, 54, 8, 8, "HOME", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
}

void drawGameHomePressed(TFT_eSPI &tft, UI &ui) {
    ui.drawRetroButton(212, 166, 96, 54, 8, 8, "HOME", 4,
                        BTN_SHADOW, TFT_BLACK, BTN_SHADOW, GB_BUTTON, TFT_WHITE);
}

void drawGameModePressed(TFT_eSPI &tft, UI &ui, const char* label) {
    ui.drawRetroButton(212, 103, 96, 54, 8, 8, label, 4,
                        BTN_SHADOW, TFT_BLACK, BTN_SHADOW, GB_BUTTON, TFT_WHITE);
}

void drawGamePlayPressed(TFT_eSPI &tft, UI &ui) {
    ui.drawRetroButton(212, 40, 96, 54, 8, 8, "PLAY", 4,
                        BTN_SHADOW, TFT_BLACK, BTN_SHADOW, GB_BUTTON, TFT_WHITE);
}

void drawGamePlayInactive(TFT_eSPI &tft, UI &ui) {
    ui.drawRetroButton(212, 40, 96, 54, 8, 8, "PLAY", 4,
                        INACTIVE_BUTTON, TFT_BLACK, INACTIVE_SHADOW, INACTIVE_GLARE, TFT_WHITE);
}

void drawGameModeInactive(TFT_eSPI &tft, UI &ui) {
    ui.drawRetroButton(212, 103, 96, 54, 8, 8, "MODE", 4,
                        INACTIVE_BUTTON, TFT_BLACK, INACTIVE_SHADOW, INACTIVE_GLARE, TFT_WHITE);
}
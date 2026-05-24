/* screens.h
 Static screen-drawing helpers for calibration, SCT, self-test, and stub views.
 These just draw. No state logic lives here. */

#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "stopwatch/stopwatch.h"
#include "../../game/game.h"

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

// Tic tac toe grid
static constexpr int GAME_SIZE = 180;
static constexpr int GAME_X = 20;
static constexpr int GAME_Y = 40;
static constexpr int GAME_COL1_X = GAME_X + (GAME_SIZE/6);
static constexpr int GAME_COL2_X = GAME_X + (GAME_SIZE/6) + (GAME_SIZE/3);
static constexpr int GAME_COL3_X = GAME_X + (GAME_SIZE/6) + (2*GAME_SIZE/3);
static constexpr int GAME_ROW1_Y = GAME_Y + (GAME_SIZE/6);
static constexpr int GAME_ROW2_Y = GAME_Y + (GAME_SIZE/6) + (GAME_SIZE/3);
static constexpr int GAME_ROW3_Y = GAME_Y + (GAME_SIZE/6) + (2*GAME_SIZE/3);
static constexpr std::pair<int, int> CELL_COORDS[3][3] = {{{GAME_COL1_X, GAME_ROW1_Y}, {GAME_COL2_X, GAME_ROW1_Y}, {GAME_COL3_X, GAME_ROW1_Y}},
                                                        {{GAME_COL1_X, GAME_ROW2_Y}, {GAME_COL2_X, GAME_ROW2_Y}, {GAME_COL3_X, GAME_ROW2_Y}},
                                                        {{GAME_COL1_X, GAME_ROW3_Y}, {GAME_COL2_X, GAME_ROW3_Y}, {GAME_COL3_X, GAME_ROW3_Y}}};
static constexpr int CROSS_LENGTH = 55;
static constexpr int CIRCLE_RADIUS = 22;

void drawCalibrationScreen(TFT_eSPI &tft);
void drawCalibrationDone(TFT_eSPI &tft, bool isReentry, uint32_t savedSteps);
void drawCalibrationGuided(TFT_eSPI &tft, const char *dirLabel,
                           bool isSampling, int secsLeft, int dirIndex);

void drawSWTime(TFT_eSPI &tft, const Stopwatch::SW_Time &t);
void drawSWScreen(TFT_eSPI &tft, Stopwatch &sw);
void updateSWScreen(TFT_eSPI &tft, Stopwatch &sw);
void eraseSWDot(TFT_eSPI &tft, Stopwatch &sw);
void drawSWDot(TFT_eSPI &tft, Stopwatch &sw);

void drawSelfTestScreen(TFT_eSPI &tft, bool passed, const char *resultStr,
                        float dX, float dY, float dZ);

void drawStubScreen(TFT_eSPI &tft, const char *title);

void drawGameScreen(TFT_eSPI &tft, UI &ui);
std::pair<int, int> getCellXY(int row, int col);
void drawGameX(TFT_eSPI &tft, int row, int col);
void drawGameO(TFT_eSPI &tft, int row, int col);

// Game buttons
void drawGamePlay(TFT_eSPI &tft, UI &ui);
void drawGameMode(TFT_eSPI &tft, UI &ui);
void drawGameHome(TFT_eSPI &tft, UI &ui);
void drawGamePlayPressed(TFT_eSPI &tft, UI &ui);
void drawGameModePressed(TFT_eSPI &tft, UI &ui);
void drawGamePlayInactive(TFT_eSPI &tft, UI &ui);
void drawGameModeInactive(TFT_eSPI &tft, UI &ui);
void drawGameHomePressed(TFT_eSPI &tft, UI &ui);

// setup wizard screens
void drawSetupWelcome(TFT_eSPI &tft);
void drawSetupQuestion(TFT_eSPI &tft, const char *question,
                       const char *unit, float value, int decimals);
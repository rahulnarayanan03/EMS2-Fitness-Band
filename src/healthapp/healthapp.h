// healthapp.h
// Health goals + progress tracking for EMS2.
//
// Three sub-screens:
//   HEALTH_MENU     - pick between setting goals or checking progress
//   HEALTH_GOALS    - adjust kcal/steps/active mins targets, save to NVS
//   HEALTH_PROGRESS - three bars showing how close you are to each goal

#ifndef HEALTHAPP_H
#define HEALTHAPP_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "../display/UI/UI.h"

// active sub-screen
enum class HealthScreen {
    HEALTH_MENU,
    HEALTH_GOALS,
    HEALTH_PROGRESS
};

// which row is selected on the goals screen
enum class GoalField {
    FIELD_CALORIES,
    FIELD_STEPS,
    FIELD_MINUTES
};

class HealthApp {
public:
    HealthApp(TFT_eSPI &tft, UI &ui);

    // call this when switching to CASE_HEALTH - reloads goals and draws the menu
    void begin();

    // call every loop tick - handles touch input for whichever sub-screen is up
    void update(uint16_t tx, uint16_t ty, bool touched);

    // called from the home reset button - clears progress but leaves goals alone
    void resetProgress();

    // call every loop to track running time. pass true when pace == RUNNING
    void tickActivity(bool isRunning, uint32_t nowMs);

    // feed in the latest step count and calorie count from main
    void setCurrentSteps(uint32_t steps);
    void setCurrentCalories(float kcal);

    // main checks this to know when to goHome()
    bool wantsHome() const;
    void clearHomeFlag();

private:
    TFT_eSPI &_tft;
    UI       &_ui;

    HealthScreen _screen    = HealthScreen::HEALTH_MENU;
    GoalField    _field     = GoalField::FIELD_CALORIES;
    bool         _wantsHome = false;

    // goals - these get saved to NVS so they survive reboots
    uint32_t _goalCalories = 500;
    uint32_t _goalSteps    = 10000;
    uint32_t _goalMinutes  = 30;

    // current session progress - reset when user hits the home screen reset button
    uint32_t _curSteps    = 0;
    float    _curCalories = 0.0f;
    uint32_t _curMinutes  = 0;

    // running time tracking
    bool     _wasRunning  = false;
    uint32_t _runStartMs  = 0;
    uint32_t _runAccumMs  = 0;

    void loadGoals();
    void saveGoals();

    void drawMenu();
    void drawGoalsScreen();
    void drawProgressScreen();

    void drawGoalValue(GoalField field);
    void drawFieldHighlight(GoalField field, bool selected);
    void drawBar(int16_t x, int16_t y, int16_t w, int16_t h,
                 uint32_t current, uint32_t goal, uint16_t fillCol);

    void handleMenuTouch(uint16_t tx, uint16_t ty);
    void handleGoalsTouch(uint16_t tx, uint16_t ty);
    void handleProgressTouch(uint16_t tx, uint16_t ty);

    bool    isHomeTouch(uint16_t tx, uint16_t ty) const;
    int16_t fieldY(GoalField field) const;
};

#endif
/* main.cpp
 State machine coordinator for the EMS2 fitness band.

 Cases:
   0 = SETUP    - first-boot profile setup (height, weight, age)
   1 = C.T      - manual calibration test
   2 = HOME     - main homepage, Red's sprite, steps, app buttons
   3 = S.T      - self test
   4 = S.C.T    - step count test
   5 = P.ID.T   - pace ID test
   6 = SETTINGS - edit age, weight, height from gear icon in top bar
   7 = HEALTH   - health goals and progress tracking
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include "calibration/calibration.h"
#include "steptrack/step_counter.h"
#include "display/UI/UI.h"
#include "display/screens/screens.h"
#include "pacefind/pacefind.h"
#include "selftest/SelfTest.h"
#include "calories/calories.h"
#include "Adafruit_MAX1704X.h"
#include "stopwatch/stopwatch.h"
#include "healthapp/healthapp.h"

Adafruit_MAX17048 maxlipo;

// ---- hardware ---------------------------------------------------------------

TFT_eSPI tft = TFT_eSPI();

#define TOUCH_CS  33
#define TOUCH_IRQ 36

static constexpr int ST_PIN = 4;
static constexpr int X_PIN = 34;
static constexpr int Y_PIN = 35;
static constexpr int Z_PIN = 26;

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700;
uint16_t touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

// ---- display brightness -----------------------------------------------------

static constexpr uint8_t  SCREEN_BRIGHTNESS_PERCENT = 100;
static constexpr uint32_t BACKLIGHT_PWM_FREQ         = 5000;
static constexpr uint8_t  BACKLIGHT_PWM_RES_BITS     = 8;
static constexpr uint8_t  BACKLIGHT_PWM_CHANNEL      = 0;

// ---- module objects ---------------------------------------------------------
using namespace SW_Consts;

Calibration calibM;
StepCounter stepM(calibM);
PACEFIND    paceM;
SelfTest    stM(ST_PIN, X_PIN, Y_PIN, Z_PIN);
Calories    calorieM;
UI          ui(tft, stepM, calibM);
Stopwatch   sw(Stopwatch::SW_PERIOD_MS);
Game        game;
HealthApp   healthApp(tft, ui);

// ---- shared app screen colours ---------------------------------------------

static constexpr uint16_t APP_BG          = TFT_BLACK;
static constexpr uint16_t APP_TEXT        = TFT_WHITE;
static constexpr uint16_t APP_MUTED       = TFT_LIGHTGREY;
static constexpr uint16_t APP_BUTTON      = GB_LIGHTEST;
static constexpr uint16_t APP_BUTTON_TEXT = GB_DARKEST;
static constexpr uint16_t APP_BORDER      = GB_DARKEST;

// ---- state machine ----------------------------------------------------------

enum AppCase {
    CASE_SETUP,
    CASE_CT,
    CASE_HOME,
    CASE_ST,
    CASE_SW,
    CASE_PIDT,
    CASE_GAME,
    CASE_HEALTH,
    CASE_SETTINGS
};

AppCase currentCase = CASE_SETUP;

bool     caseCtIsReentry     = false;
uint32_t savedStepsBeforeCal = 0;
bool     awaitingStepChoice  = false;

uint16_t tx = 0, ty = 0;
bool     touched = false;
bool step_reset_touched = false;
bool game_play_touched = false;
bool game_mode_touched = false;
bool game_home_touched = false;
bool game_ended = false;
bool game_buttons_pressable = true;
bool game_mode_active = false;
unsigned long game_mode_timer = 0;
bool idle_anim_updated = true;
int bot_delay_start;
int pressed_time = 0;
int idle_anim_time = 0;
int idle_anim_delay_ms = 300;
int current_anim_index = 0;
int tx_shift = 20;

uint32_t lastTransition = 0;

// ---- setup wizard state -----------------------------------------------------

enum SetupStep {
    SETUP_WELCOME,
    SETUP_HEIGHT,
    SETUP_WEIGHT,
    SETUP_AGE,
    SETUP_DONE
};

SetupStep setupStep   = SETUP_WELCOME;
float     setupHeight = 170.0f;
float     setupWeight = 70.0f;
int       setupAge    = 25;

// ---- settings state ---------------------------------------------------------

SetupStep settingsStep   = SETUP_HEIGHT;
float     settingsHeight = 170.0f;
float     settingsWeight = 70.0f;
int       settingsAge    = 25;

void initCalibration();

// ---- screen redraw flags ----------------------------------------------------

bool paceIdDrawn = false;

// ---- helpers ----------------------------------------------------------------

bool readTouch(uint16_t &tx, uint16_t &ty) {
    if (!ts.touched()) return false;

    long sumX = 0;
    long sumY = 0;
    int count = 0;

    for (int i = 0; i < 5; i++) {
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            sumX += p.x;
            sumY += p.y;
            count++;
        }

        delay(10);
    }

    if (count == 0) return false;

    tx = map(sumX / count, touchScreenMinimumX, touchScreenMaximumX, 0, tft.width());
    ty = map(sumY / count, touchScreenMinimumY, touchScreenMaximumY, 0, tft.height());

    return true;
}

void setScreenBrightness(uint8_t percent) {
    percent = constrain(percent, 0, 100);

#if defined(TFT_BL)
    uint8_t duty = map(percent, 0, 100, 0, 255);

    #if defined(TFT_BACKLIGHT_ON) && TFT_BACKLIGHT_ON == LOW
        duty = 255 - duty;
    #endif

    #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(TFT_BL, duty);
    #else
        ledcWrite(BACKLIGHT_PWM_CHANNEL, duty);
    #endif
#else
    Serial.println("[Display] TFT_BL is not defined, so backlight brightness cannot be controlled in software.");
#endif
}

void initBacklight() {
#if defined(TFT_BL)
    #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcAttach(TFT_BL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RES_BITS);
    #else
        ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RES_BITS);
        ledcAttachPin(TFT_BL, BACKLIGHT_PWM_CHANNEL);
    #endif

    setScreenBrightness(SCREEN_BRIGHTNESS_PERCENT);

    Serial.printf("[Display] Backlight brightness set to %u%%.\n", SCREEN_BRIGHTNESS_PERCENT);
#else
    Serial.println("[Display] TFT_BL is not defined. Check TFT_eSPI User_Setup if the backlight pin is wired to ESP32.");
#endif
}

void drawPaceIdHomeButton() {
    ui.drawRetroButton(95, 184, 130, 48, 10, 6, "HOME", 4,
                        GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
}

void goHome() {
    stepM.resumeCounting();

    currentCase    = CASE_HOME;
    lastTransition = millis();

    paceIdDrawn = false;

    ui.begin();
}

void updateStepAndPace(uint32_t now) {
    if (!calibM.isCalibrated()) return;

    stepM.update();

    if (stepM.wasStepDetected()) {
        paceM.update(now);
        calorieM.onStep(paceM.getPace());
    } else {
        paceM.checkTimeout(now);
    }
}

// ---- case functions ---------------------------------------------------------

void Setup_Case() {
    if (!touched) return;

    switch (setupStep) {
        case SETUP_WELCOME:
            setupStep      = SETUP_HEIGHT;
            lastTransition = millis();
            drawSetupQuestion(tft, "What is your height?", "cm", setupHeight, 0);
            break;

        case SETUP_HEIGHT:
            if (tx >= 20 && tx <= 90 && ty >= 155 && ty <= 205) {
                setupHeight = max(100.0f, setupHeight - 1.0f);
                drawSetupQuestion(tft, "What is your height?", "cm", setupHeight, 0);
            } else if (tx >= 230 && tx <= 300 && ty >= 155 && ty <= 205) {
                setupHeight = min(220.0f, setupHeight + 1.0f);
                drawSetupQuestion(tft, "What is your height?", "cm", setupHeight, 0);
            } else if (tx >= 110 && tx <= 210 && ty >= 155 && ty <= 205) {
                setupStep      = SETUP_WEIGHT;
                lastTransition = millis();
                drawSetupQuestion(tft, "What is your weight?", "kg", setupWeight, 1);
            }
            break;

        case SETUP_WEIGHT:
            if (tx >= 20 && tx <= 90 && ty >= 155 && ty <= 205) {
                setupWeight = max(30.0f, setupWeight - 0.5f);
                drawSetupQuestion(tft, "What is your weight?", "kg", setupWeight, 1);
            } else if (tx >= 230 && tx <= 300 && ty >= 155 && ty <= 205) {
                setupWeight = min(200.0f, setupWeight + 0.5f);
                drawSetupQuestion(tft, "What is your weight?", "kg", setupWeight, 1);
            } else if (tx >= 110 && tx <= 210 && ty >= 155 && ty <= 205) {
                setupStep      = SETUP_AGE;
                lastTransition = millis();
                drawSetupQuestion(tft, "What is your age?", "yrs", (float)setupAge, 0);
            }
            break;

        case SETUP_AGE:
            if (tx >= 20 && tx <= 90 && ty >= 155 && ty <= 205) {
                setupAge = max(10, setupAge - 1);
                drawSetupQuestion(tft, "What is your age?", "yrs", (float)setupAge, 0);
            } else if (tx >= 230 && tx <= 300 && ty >= 155 && ty <= 205) {
                setupAge = min(100, setupAge + 1);
                drawSetupQuestion(tft, "What is your age?", "yrs", (float)setupAge, 0);
            } else if (tx >= 110 && tx <= 210 && ty >= 155 && ty <= 205) {
                calorieM.saveProfile(setupAge, setupWeight, setupHeight);

                setupStep = SETUP_DONE;

                Serial.printf("[Setup] Done - age=%d weight=%.1f height=%.1f\n",
                              setupAge, setupWeight, setupHeight);

                goHome();
            }
            break;

        default:
            break;
    }
}

void Settings_Case() {
    if (!touched) return;

    switch (settingsStep) {
        case SETUP_HEIGHT:
            if (tx >= 20 && tx <= 90 && ty >= 155 && ty <= 205) {
                settingsHeight = max(100.0f, settingsHeight - 1.0f);
                drawSetupQuestion(tft, "Height", "cm", settingsHeight, 0);
            } else if (tx >= 230 && tx <= 300 && ty >= 155 && ty <= 205) {
                settingsHeight = min(220.0f, settingsHeight + 1.0f);
                drawSetupQuestion(tft, "Height", "cm", settingsHeight, 0);
            } else if (tx >= 110 && tx <= 210 && ty >= 155 && ty <= 205) {
                settingsStep   = SETUP_WEIGHT;
                lastTransition = millis();
                drawSetupQuestion(tft, "Weight", "kg", settingsWeight, 1);
            }
            break;

        case SETUP_WEIGHT:
            if (tx >= 20 && tx <= 90 && ty >= 155 && ty <= 205) {
                settingsWeight = max(30.0f, settingsWeight - 0.5f);
                drawSetupQuestion(tft, "Weight", "kg", settingsWeight, 1);
            } else if (tx >= 230 && tx <= 300 && ty >= 155 && ty <= 205) {
                settingsWeight = min(200.0f, settingsWeight + 0.5f);
                drawSetupQuestion(tft, "Weight", "kg", settingsWeight, 1);
            } else if (tx >= 110 && tx <= 210 && ty >= 155 && ty <= 205) {
                settingsStep   = SETUP_AGE;
                lastTransition = millis();
                drawSetupQuestion(tft, "Age", "yrs", (float)settingsAge, 0);
            }
            break;

        case SETUP_AGE:
            if (tx >= 20 && tx <= 90 && ty >= 155 && ty <= 205) {
                settingsAge = max(10, settingsAge - 1);
                drawSetupQuestion(tft, "Age", "yrs", (float)settingsAge, 0);
            } else if (tx >= 230 && tx <= 300 && ty >= 155 && ty <= 205) {
                settingsAge = min(100, settingsAge + 1);
                drawSetupQuestion(tft, "Age", "yrs", (float)settingsAge, 0);
            } else if (tx >= 110 && tx <= 210 && ty >= 155 && ty <= 205) {
                calorieM.saveProfile(settingsAge, settingsWeight, settingsHeight);
                lastTransition = millis();

                Serial.printf("[Settings] Updated - age=%d weight=%.1f height=%.1f\n",
                              settingsAge, settingsWeight, settingsHeight);

                goHome();
            }
            break;

        default:
            goHome();
            break;
    }
}

void Calibration_Case() {
    calibM.update();

    if (!awaitingStepChoice && !calibM.isCalibrated()) {
        static Calibration::Stage lastStage = Calibration::Stage::IDLE;
        static int                lastSecs  = -1;
        static int                lastDir   = -1;

        auto stage    = calibM.getStage();
        int  secs     = calibM.getSecsLeft();
        int  dir      = calibM.getDirIndex();
        bool sampling = (stage == Calibration::Stage::SAMPLING);

        if (stage != lastStage || secs != lastSecs || dir != lastDir) {
            drawCalibrationGuided(tft,
                                  Calibration::DIR_LABEL[dir],
                                  sampling,
                                  secs,
                                  dir);
            lastStage = stage;
            lastSecs  = secs;
            lastDir   = dir;
        }
    }

    if (!awaitingStepChoice && calibM.isCalibrated()) {
        awaitingStepChoice = true;
        drawCalibrationDone(tft, ui, caseCtIsReentry, savedStepsBeforeCal);
        Serial.println("Calibration complete.");
    }

    if (awaitingStepChoice && touched) {
        if (caseCtIsReentry) {
            if (tx >= 35 && tx <= 150 && ty >= 184 && ty <= 232) {
                Serial.println("CT: keeping previous step count.");

                awaitingStepChoice = false;
                caseCtIsReentry    = false;

                goHome();

            } else if (tx >= 170 && tx <= 285 && ty >= 184 && ty <= 232) {
                stepM.resetCount();
                Serial.println("CT: step count reset to 0.");

                awaitingStepChoice = false;
                caseCtIsReentry    = false;

                goHome();
            }

        } else {
            if (tx >= 110 && tx <= 210 && ty >= 190 && ty <= 230) {
                awaitingStepChoice = false;
                goHome();
            }
        }
    }
}

void Home_Case(uint32_t now, float cv, float cp) {
    static bool homeTouchHandled = false;

    updateStepAndPace(now);

    // keep healthApp in sync with current data even while on home screen
    bool isRunning = strcmp(paceM.getPace(), "RUNNING") == 0;
    healthApp.tickActivity(isRunning, now);
    healthApp.setCurrentSteps(stepM.getStepCount());
    healthApp.setCurrentCalories(calorieM.getKcal());

    ui.setTime(0, 0);
    ui.setDate(1, 1);
    ui.setPace(paceM.getPace());
    ui.setCalories(calorieM.getKcal());
    ui.update(now, cv, cp);

    if (step_reset_touched) {
        // show the reset button as pressed for 100ms
        if (millis() - pressed_time > 100) {
            ui.drawStepResetButton();
            step_reset_touched = false;
        }
    }

    if (!touched) {
        homeTouchHandled = false;
        return;
    }

    if (homeTouchHandled) return;
    homeTouchHandled = true;

    if (ui.checkSettingsTouch(tx, ty)) {
        settingsHeight = calorieM.getHeightCm();
        settingsWeight = calorieM.getWeightKg();
        settingsAge    = calorieM.getAge();
        settingsStep   = SETUP_HEIGHT;
        lastTransition = millis();
        currentCase    = CASE_SETTINGS;

        drawSetupQuestion(tft, "Height", "cm", settingsHeight, 0);
        return;
    }

    if (ui.checkStepResetTouch(tx, ty)) {
        stepM.resetCount();
        calorieM.reset();
        healthApp.resetProgress();

        lastTransition = millis();

        Serial.println("Home: step count reset to 0.");

        // start tracking time since touch
        pressed_time = millis();
        step_reset_touched = true;

        // draw pressed reset button
        ui.drawStepResetButtonPressed();
        return;
    }

    uint8_t btnIndex = 0;

    if (ui.checkButtonTouch(tx, ty, btnIndex)) {
        lastTransition = millis();

        switch (btnIndex) {
            case 0:
                stepM.saveNow();
                stepM.pauseCounting();

                savedStepsBeforeCal = stepM.getStepCount();
                caseCtIsReentry     = true;
                awaitingStepChoice  = false;
                currentCase         = CASE_CT;

                calibM.startCalibration();
                calorieM.reset();
                drawCalibrationScreen(tft);

                Serial.println("Re-entering calibration.");
                break;

            case 1:
                currentCase = CASE_ST;
                break;

            case 2:
                currentCase = CASE_GAME;
                current_anim_index = 0;
                idle_anim_updated = true;
                idle_anim_time = 0;
                drawGameScreen(tft, ui);
                break;

            case 3:
                currentCase = CASE_PIDT;
                paceIdDrawn = false;
                tft.fillScreen(APP_BG);
                break;

            case 4:
                currentCase = CASE_SW;
                drawSWScreen(tft, sw, ui);
                break;

            case 5:
                currentCase = CASE_HEALTH;
                healthApp.begin();
                break;
        }
    }
}

void SelfTest_Case() {
    static bool stRan = false;

    if (!stRan) {
        stRan = true;

        bool passed = stM.run();

        drawSelfTestScreen(tft, ui,
                           passed,
                           stM.getResultStr(),
                           stM.getDeltaX(),
                           stM.getDeltaY(),
                           stM.getDeltaZ());
    }

    if (touched && tx >= 110 && tx <= 210 && ty >= 190 && ty <= 230) {
        stRan = false;
        goHome();
    }
}

void StopWatch_Case(uint32_t now) {
    Stopwatch::SW_State state = sw.getState();

    static unsigned long lastSWUpdate = 0;

    // Touch edge detection
    // This makes a held press count only once.
    static bool wasTouched = false;
    bool newTouch = touched && !wasTouched;
    wasTouched = touched;

    if ((millis() - lastSWUpdate >= Stopwatch::SW_DELAY) && sw.getState() == Stopwatch::RUNNING) {
        lastSWUpdate = millis();

        sw.updateSW();
        updateSWScreen(tft, sw);

        Serial.print("[SW] Elapsed time: ");
        Serial.print(sw.getElapsedTimeSeconds());
        Serial.print("s, State: ");
        Serial.println(state);
        Serial.print("Point x: ");
        Serial.print(sw.getCirclePosition().first);
        Serial.print(", Point y: ");
        Serial.println(sw.getCirclePosition().second);
    }

    if (newTouch && sw.homeTouched(tx, ty)) {
        goHome();

    } else if (newTouch && sw.startStopTouched(tx, ty)) {
        Serial.println("Start/stop touched");

        if (state == Stopwatch::RUNNING) {
            sw.stopSW();
            Serial.println("SW stopped");

            tft.setTextDatum(TL_DATUM);
            ui.drawRetroButton(SW_BTN_X - SW_BTN_R, START_Y - SW_BTN_R,
                               2 * SW_BTN_R, 2 * SW_BTN_R,
                               6, 6, "START", 2,
                               GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
        } else {
            sw.startSW();
            Serial.println("SW started");

            tft.setTextDatum(TL_DATUM);
            ui.drawRetroButton(SW_BTN_X - SW_BTN_R, START_Y - SW_BTN_R,
                               2 * SW_BTN_R, 2 * SW_BTN_R,
                               6, 6, "STOP", 2,
                               RESET_RED, TFT_BLACK, RESET_SHADOW, RESET_GLARE, TFT_WHITE);
        }

    } else if (newTouch && sw.resetTouched(tx, ty)) {
        Serial.println("Reset touched");

        eraseSWDot(tft, sw);

        // Stop and reset even if the stopwatch is running
        sw.resetSW();

        drawSWDot(tft, sw);
        drawSWTime(tft, sw.getFormattedTime());

        tft.setTextDatum(TL_DATUM);
        ui.drawRetroButton(SW_BTN_X - SW_BTN_R, START_Y - SW_BTN_R,
                           2 * SW_BTN_R, 2 * SW_BTN_R,
                           6, 6, "START", 2,
                           GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
    }
}

void PaceID_Case() {
    if (!paceIdDrawn) {
        paceIdDrawn = true;

        tft.fillScreen(APP_BG);

        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(APP_TEXT, APP_BG);
        tft.drawString("PACE ID TEST", 10, 10, 4);

        drawPaceIdHomeButton();
    }

    if (calibM.isCalibrated()) {
        stepM.update();

        uint32_t now = millis();

        if (stepM.wasStepDetected()) {
            paceM.update(now);
        } else {
            paceM.checkTimeout(now);
        }
    }

    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 500) {
        lastUpdate = millis();

        tft.fillRect(10, 58, 300, 120, APP_BG);

        tft.setTextDatum(TL_DATUM);

        tft.setTextColor(APP_MUTED, APP_BG);
        tft.drawString("Pace:", 10, 58, 4);

        tft.setTextColor(APP_TEXT, APP_BG);
        tft.drawString(paceM.getPace(), 10, 92, 4);

        tft.setTextColor(APP_MUTED, APP_BG);
        tft.drawString("Steps:", 10, 130, 4);

        char buf[12];
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)stepM.getStepCount());

        tft.setTextColor(APP_TEXT, APP_BG);
        tft.drawString(buf, 120, 130, 4);
    }

    if (touched && tx >= 95 && tx <= 225 && ty >= 184 && ty <= 232) {
        paceIdDrawn = false;
        goHome();
    }
}

void Health_Case() {
    // keep accumulating running time even while in the health screen
    bool isRunning = strcmp(paceM.getPace(), "RUNNING") == 0;
    healthApp.tickActivity(isRunning, millis());
    healthApp.setCurrentSteps(stepM.getStepCount());
    healthApp.setCurrentCalories(calorieM.getKcal());

    healthApp.update(tx, ty, touched);

    if (healthApp.wantsHome()) {
        healthApp.clearHomeFlag();
        goHome();
    }
}

void Game_Case() {
    // if the play button gets touched
    if (game_buttons_pressable && touched && tx >= 212 && tx <= 307 && ty >= 40 && ty <= 93) {
        game_buttons_pressable = false;
        pressed_time = millis();
        game_play_touched = true;
        drawGamePlayPressed(tft, ui);
    }

    // if the mode button gets touched
    if (game_buttons_pressable && touched && tx >= 212 && tx <= 307 && ty >= 103 && ty <= 156) {
        pressed_time = millis();
        game_mode_touched = true;

        // show pressed state with the current label
        const char* pressedLabel = "MODE";
        if (game_mode_active) {
            Game::Difficulty d = game.getDifficulty();
            if (d == Game::EASY) {
                pressedLabel = "EASY";
            } else if (d == Game::MEDIUM) {
                pressedLabel = "MED";
            } else if (d == Game::HARD) {
                pressedLabel = "HARD";
            }
        }
        drawGameModePressed(tft, ui, pressedLabel);
    }

    // if the home button gets touched
    if (touched && tx >= 212 && tx <= 307 && ty >= 166 && ty <= 219) {
        pressed_time = millis();
        game_home_touched = true;
        drawGameHomePressed(tft, ui);
    }

    // show the play button as pressed for 100ms
    if (game_play_touched) {
        if (millis() - pressed_time > 100) {
            game_play_touched = false;
            // draw game play and mode buttons as inactive
            drawGameScreen(tft, ui);
            drawGamePlayInactive(tft, ui);
            drawGameModeInactive(tft, ui);
            game.playGame();
            game_ended = false;
        }
    }

    // show the mode button as pressed for 100ms
    if (game_mode_touched) {
        if (millis() - pressed_time > 100) {
            game_mode_touched = false;
            
            if (game_mode_active) {
                // cycle through difficulties
                Game::Difficulty difficulty = game.getDifficulty();
                switch (difficulty) {
                    case Game::EASY:
                        game.setDifficulty(Game::MEDIUM);
                        break;
                    case Game::MEDIUM:
                        game.setDifficulty(Game::HARD);
                        break;
                    case Game::HARD:
                        game.setDifficulty(Game::EASY);
                        break;
                    default:
                        game.setDifficulty(Game::EASY);
                        break;
                }
            }

            // restart the 2s window and show current difficulty
            game_mode_active = true;
            game_mode_timer = millis();

            Game::Difficulty difficulty = game.getDifficulty();
            const char* label = "EASY";
            if (difficulty == Game::MEDIUM) {
                label = "MED";
            } else if (difficulty == Game::HARD) {
                label = "HARD";
            }

            ui.drawRetroButton(212, 103, 96, 54, 8, 8, label, 4,
                            GB_BUTTON, TFT_BLACK, BTN_SHADOW, BTN_GLARE, TFT_WHITE);
        }
    }

    // after 2s with no mode press, revert button label to "MODE"
    if (game_mode_active && (millis() - game_mode_timer >= 2000)) {
        game_mode_active = false;
        // only redraw as active if not mid-game
        if (game.getGameState() == Game::IDLE || 
            game.getGameState() == Game::HUMAN_WIN || 
            game.getGameState() == Game::BOT_WIN || 
            game.getGameState() == Game::DRAW) {
            drawGameMode(tft, ui);
        } else {
            drawGameModeInactive(tft, ui);
        }
    }

    // show the home button as pressed for 100ms
    if (game_home_touched) {
        if (millis() - pressed_time > 100) {
            current_anim_index = 0;
            idle_anim_updated = true;
            idle_anim_time = 0;
            game_home_touched = false;
            game.resetGame();
            game_buttons_pressable = true;
            goHome();
        }
    }

    // handle cells being touched
    if (touched) {
        std::pair<int, int> cell_touched;
        cell_touched = game.getRowColTouched((tx - tx_shift), ty);
        int touched_row = cell_touched.first;
        int touched_col = cell_touched.second;
        if (touched_row == 0 || touched_col == 0) {
            return;
        } else {
            // a valid cell has been touched, make sure it is the human's turn and the cell isn't occupied
            if (game.getGameState() == Game::HUMAN_TURN && game.getCellState(touched_row, touched_col) == Game::FREE) {
                Serial.print("Valid touch detected on row ");
                Serial.print(touched_row);
                Serial.print(", column ");
                Serial.println(touched_col);
                // place an 'X' on the cell
                game.placeX(touched_row, touched_col);
                drawGameX(tft, touched_row, touched_col);
                if (game.checkWin(Game::HUMAN)) {
                    Serial.println("Human has won");
                    // render human win title
                    tft.fillRect(0, 0, 320, 35, GB_LIGHTEST);
                    tft.setTextDatum(CC_DATUM);
                    tft.setTextColor(TFT_BLACK);
                    tft.drawString("YOU WIN!", 160, 20, 4);
                    tft.setTextDatum(TL_DATUM);
                    tft.drawFastHLine(100, 32, 120, TFT_BLACK);
                    // draw the active play and mode buttons
                    drawGamePlay(tft, ui);
                    drawGameMode(tft, ui);
                }
                bot_delay_start = millis();
            }
        }
    }

    // if it is the bot's turn to play
    if (game.getGameState() == Game::BOT_TURN) {
        Serial.println("Bot's turn to play");
        // check if the bot delay time has passed
        if (millis() - bot_delay_start >= Game::BOT_DELAY_MS) {
            Serial.println("Bot now placing");

            // move according to the difficulty level
            Game::Difficulty difficulty = game.getDifficulty();
            switch (difficulty) {
                case Game::EASY:
                    game.runBotMoveEasy();
                    break;
                case Game::MEDIUM:
                    game.runBotMoveMedium();
                    break;
                case Game::HARD:
                    game.runBotMoveHard();
                    break;
                default:
                    game.runBotMoveEasy();
                    break;
            }
        
            std::pair<int, int> last_bot_move = game.getLastBotMove();
            int last_bot_row = last_bot_move.first;
            int last_bot_col = last_bot_move.second;
            drawGameO(tft, last_bot_row, last_bot_col);

            // check for bot win after placing
            if (game.checkWin(Game::BOT)) {
                Serial.println("Bot has won");
                // render bot win title
                tft.fillRect(0, 0, 320, 35, GB_LIGHTEST);
                tft.setTextDatum(CC_DATUM);
                tft.setTextColor(TFT_BLACK);
                tft.drawString("BOT WINS!", 160, 20, 4);
                tft.setTextDatum(TL_DATUM);
                tft.drawFastHLine(100, 32, 120, TFT_BLACK);
                // draw the active play and mode buttons
                drawGamePlay(tft, ui);
                drawGameMode(tft, ui);
            }
        }
    }

    // if the bot or human has won, game is over
    if (!game_ended && (game.checkWin(Game::BOT) || game.checkWin(Game::HUMAN))) {
        game_ended = true;
        drawGamePlay(tft, ui);
        drawGameMode(tft, ui);
        game_buttons_pressable = true;

    // if the grid is full but nobody has won, it is a draw
    } else if (game.getGameState() != Game::DRAW && game.isBoardFull()) {
        game_ended = true;
        drawGamePlay(tft, ui);
        drawGameMode(tft, ui);
        game_buttons_pressable = true;

        // render draw title
        tft.fillRect(0, 0, 320, 35, GB_LIGHTEST);
        tft.setTextDatum(CC_DATUM);
        tft.setTextColor(TFT_BLACK);
        tft.drawString("DRAW", 160, 20, 4);
        tft.setTextDatum(TL_DATUM);
        tft.drawFastHLine(110, 32, 100, TFT_BLACK);
    }

    // idle animation
    if (game.getGameState() == Game::IDLE) {
        // if the idle animation just got updated, start tracking time
        if (idle_anim_updated) {
            idle_anim_updated = false;
            idle_anim_time = millis();

        // if the time has passed for the next animation frame, render it
        } else if (millis() - idle_anim_time >= idle_anim_delay_ms) {
            idle_anim_updated = true;

            if (current_anim_index < 11 && current_anim_index > 0) {
                std::pair<int, int> circle_coords = getCellXY(anim_path[current_anim_index].first,
                                                            anim_path[current_anim_index].second);
                int circle_x = circle_coords.first;
                int circle_y = circle_coords.second;
                
                tft.fillCircle(circle_x, circle_y, 18, GB_BUTTON);

                std::pair<int, int> circle_coords_prev = getCellXY(anim_path[current_anim_index-1].first,
                                                                anim_path[current_anim_index-1].second);
                int circle_x_prev = circle_coords_prev.first;
                int circle_y_prev = circle_coords_prev.second;
                
                tft.fillCircle(circle_x_prev, circle_y_prev, 18, GB_LIGHTEST);

                current_anim_index++;

            } else if (current_anim_index == 11) {
                std::pair<int, int> circle_coords = getCellXY(anim_path[current_anim_index].first,
                                                            anim_path[current_anim_index].second);
                int circle_x = circle_coords.first;
                int circle_y = circle_coords.second;
                
                tft.fillCircle(circle_x, circle_y, 18, GB_BUTTON);

                std::pair<int, int> circle_coords_prev = getCellXY(anim_path[current_anim_index-1].first,
                                                                anim_path[current_anim_index-1].second);
                int circle_x_prev = circle_coords_prev.first;
                int circle_y_prev = circle_coords_prev.second;
                
                tft.fillCircle(circle_x_prev, circle_y_prev, 18, GB_LIGHTEST);

                current_anim_index = 0;

            } else {
                std::pair<int, int> circle_coords = getCellXY(anim_path[current_anim_index].first,
                                                            anim_path[current_anim_index].second);
                int circle_x = circle_coords.first;
                int circle_y = circle_coords.second;
                
                tft.fillCircle(circle_x, circle_y, 18, GB_BUTTON);

                std::pair<int, int> circle_coords_prev = getCellXY(anim_path[11].first, anim_path[11].second);
                int circle_x_prev = circle_coords_prev.first;
                int circle_y_prev = circle_coords_prev.second;
                
                tft.fillCircle(circle_x_prev, circle_y_prev, 18, GB_LIGHTEST);

                current_anim_index++;
            }
        }
    }
}

// ---- setup helpers ----------------------------------------------------------

void initDisplay() {
    tft.init();
    tft.setRotation(DISPLAY_ROTATION);

    initBacklight();

    touchSPI.begin(25, 39, 32);
    SPI.begin(25, 39, 32);

    ts.begin();
    ts.setRotation(1);
}

void initCalibration() {
    stepM.pauseCounting();

    calibM.begin();
    calibM.startCalibration();
    drawCalibrationScreen(tft);

    Serial.println("Manual calibration started.");
}

// ---- setup ------------------------------------------------------------------

void setup() {
    Serial.begin(115200);

    if (!maxlipo.begin()) {
        Serial.println("MAX17048 not found");
        while (1);
    }

    delay(500);

    Wire.begin(21, 22);

    stM.begin();
    initDisplay();

    calibM.begin();
    stepM.begin();
    calorieM.begin();

    if (calorieM.isProfileSet()) {
        currentCase = CASE_HOME;
        ui.begin();

        Serial.println("Boot: profile found, going straight to Home.");
    } else {
        currentCase = CASE_SETUP;
        setupStep   = SETUP_WELCOME;
        drawSetupWelcome(tft);

        Serial.println("Boot: no profile found, starting setup wizard.");
    }
}

// ---- loop -------------------------------------------------------------------

void loop() {
    uint32_t now = millis();

    touched = (millis() - lastTransition >= 200) && readTouch(tx, ty);

    switch (currentCase) {
        case CASE_SETUP:
            Setup_Case();
            break;

        case CASE_CT:
            Calibration_Case();
            break;

        case CASE_HOME:
            Home_Case(now, maxlipo.cellVoltage(), maxlipo.cellPercent());
            break;

        case CASE_ST:
            SelfTest_Case();
            break;

        case CASE_SW:
            StopWatch_Case(now);
            break;

        case CASE_PIDT:
            PaceID_Case();
            break;

        case CASE_GAME:
            Game_Case();
            break;

        case CASE_HEALTH:
            Health_Case();
            break;

        case CASE_SETTINGS:
            Settings_Case();
            break;
    }
}
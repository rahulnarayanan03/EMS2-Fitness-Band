/* main.cpp
 State machine coordinator for the EMS2 fitness band.

 Cases:
   0 = SETUP    - first-boot profile setup (height, weight, age)
   1 = C.T      - calibration, runs automatically on boot
   2 = HOME     - main homepage, Red's sprite, steps, app buttons
   3 = S.T      - self test
   4 = S.C.T    - step count test
   5 = P.ID.T   - pace ID test
   6 = SETTINGS - edit age, weight, height from gear icon in top bar

 Boot flow:
   Power on -> Case 0 (if no profile) OR Case 1 (if profile exists)
   -> calibration runs -> home button appears -> Case 2

 From Case 2 the user taps app buttons to reach Cases 1/3/4/5.
 Gear icon in top bar goes to Case 6.
 Every app case has a home button that returns to Case 2.
 Re-entering Case 1 saves current steps, re-calibrates, then asks
 whether to keep or reset the step count.
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

Adafruit_MAX17048 maxlipo;

// ---- hardware ---------------------------------------------------------------

TFT_eSPI tft = TFT_eSPI();

#define TOUCH_CS  33
#define TOUCH_IRQ 36
#define ST_PIN    4

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700;
uint16_t touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

// ---- display brightness -----------------------------------------------------
// This only works if TFT_BL is defined in your TFT_eSPI User_Setup.
// 70 is a safer starting point because 90 made the greens wash out.

static constexpr uint8_t  SCREEN_BRIGHTNESS_PERCENT = 70;
static constexpr uint32_t BACKLIGHT_PWM_FREQ         = 5000;
static constexpr uint8_t  BACKLIGHT_PWM_RES_BITS     = 8;
static constexpr uint8_t  BACKLIGHT_PWM_CHANNEL      = 0;

// ---- module objects ---------------------------------------------------------

Calibration calibM;
StepCounter stepM(calibM);
PACEFIND    paceM;
SelfTest    stM(calibM, ST_PIN);
Calories    calorieM;
UI          ui(tft, stepM, calibM);

// ---- shared app screen colours ---------------------------------------------

static constexpr uint16_t APP_BG          = TFT_BLACK;
static constexpr uint16_t APP_TEXT        = TFT_WHITE;
static constexpr uint16_t APP_MUTED       = TFT_LIGHTGREY;
static constexpr uint16_t APP_BUTTON      = GB_LIGHTEST;
static constexpr uint16_t APP_BUTTON_TEXT = GB_DARKEST;
static constexpr uint16_t APP_BORDER      = GB_DARKEST;

// ---- state machine ----------------------------------------------------------

enum AppCase { CASE_SETUP = 0, CASE_CT = 1, CASE_HOME = 2,
               CASE_ST = 3, CASE_SCT = 4, CASE_PIDT = 5, CASE_SETTINGS = 6 };
AppCase currentCase = CASE_SETUP;

bool     caseCtIsReentry     = false;
uint32_t savedStepsBeforeCal = 0;
bool     awaitingStepChoice  = false;

// touch state, populated once per loop tick and shared across case functions
uint16_t tx = 0, ty = 0;
bool     touched = false;

// timestamp of the last case transition - touched is ignored within 200ms of one
uint32_t lastTransition = 0;

// ---- setup wizard state -----------------------------------------------------

enum SetupStep { SETUP_WELCOME, SETUP_HEIGHT, SETUP_WEIGHT, SETUP_AGE, SETUP_DONE };
SetupStep setupStep   = SETUP_WELCOME;
float     setupHeight = 170.0f;
float     setupWeight = 70.0f;
int       setupAge    = 25;

// ---- settings state ---------------------------------------------------------

// reuses SetupStep enum but skips SETUP_WELCOME — starts at SETUP_HEIGHT
SetupStep settingsStep   = SETUP_HEIGHT;
float     settingsHeight = 170.0f;
float     settingsWeight = 70.0f;
int       settingsAge    = 25;

// forward declaration so Setup_Case() can call initCalibration()
void initCalibration();

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

    // Some TFT backlight circuits are active-low.
    // If yours is active-low, define TFT_BACKLIGHT_ON as LOW in TFT_eSPI setup.
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
    tft.fillRect(110, 190, 100, 40, APP_BUTTON);
    tft.drawRect(110, 190, 100, 40, APP_BORDER);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(APP_BUTTON_TEXT, APP_BUTTON);
    tft.drawString("HOME", 160, 210, 2);
}

// go to homepage and mark the transition time for debounce
void goHome() {
    currentCase    = CASE_HOME;
    lastTransition = millis();
    ui.begin();
}

// update step count, pace and calories if calibration is done
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
            // any tap moves to height question
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
                setupStep      = SETUP_DONE;
                lastTransition = millis();
                currentCase    = CASE_CT;

                initCalibration();

                Serial.printf("[Setup] Done — age=%d weight=%.1f height=%.1f\n",
                              setupAge, setupWeight, setupHeight);
            }
            break;

        default:
            break;
    }
}

// settings case - same +/- picker flow but pre-filled with saved values
// saves on OK at the age step and returns home
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
                // save updated profile — calorie calc will use new weight immediately
                calorieM.saveProfile(settingsAge, settingsWeight, settingsHeight);
                lastTransition = millis();

                Serial.printf("[Settings] Updated — age=%d weight=%.1f height=%.1f\n",
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

    // draw guided prompt while still running through directions
    if (!awaitingStepChoice && !calibM.isCalibrated()) {
        static Calibration::Stage lastStage = Calibration::Stage::IDLE;
        static int                lastSecs  = -1;
        static int                lastDir   = -1;

        auto stage    = calibM.getStage();
        int  secs     = calibM.getSecsLeft();
        int  dir      = calibM.getDirIndex();
        bool sampling = (stage == Calibration::Stage::SAMPLING);

        // only redraw when something actually changed to avoid flicker
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
        drawCalibrationDone(tft, caseCtIsReentry, savedStepsBeforeCal);
        Serial.println("Calibration complete.");
    }

    if (awaitingStepChoice && touched) {
        if (caseCtIsReentry) {
            if (tx >= 50 && tx <= 150 && ty >= 150 && ty <= 190) {
                Serial.println("CT: keeping previous step count.");

                awaitingStepChoice = false;
                caseCtIsReentry    = false;

                goHome();

            } else if (tx >= 170 && tx <= 270 && ty >= 150 && ty <= 190) {
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

    ui.setTime(0, 0);
    ui.setDate(1, 1);
    ui.setPace(paceM.getPace());
    ui.setCalories(calorieM.getKcal());
    ui.update(now, cv, cp);

    // Only handle the first loop tick of a press.
    // This stops a held reset button from repeatedly writing 0 to NVS.
    if (!touched) {
        homeTouchHandled = false;
        return;
    }

    if (homeTouchHandled) return;
    homeTouchHandled = true;

    // check gear icon first — it sits in the top bar
    if (ui.checkSettingsTouch(tx, ty)) {
        // pre-fill settings with current saved values
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

        lastTransition = millis();

        Serial.println("Home: step count reset to 0.");
        return;
    }

    uint8_t btnIndex = 0;

    if (ui.checkButtonTouch(tx, ty, btnIndex)) {
        lastTransition = millis();

        switch (btnIndex) {
            case 0:
                stepM.saveNow();
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
                currentCase = CASE_SCT;
                drawSCTScreen(tft);
                break;

            case 3:
                currentCase = CASE_PIDT;
                break;
        }
    }
}

void SelfTest_Case() {
    static bool stRan = false;

    if (!stRan) {
        stRan = true;

        bool passed = stM.run();

        drawSelfTestScreen(tft,
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

void StepCountTest_Case(uint32_t now) {
    updateStepAndPace(now);

    static unsigned long lastSCTUpdate = 0;

    if (millis() - lastSCTUpdate >= 500) {
        lastSCTUpdate = millis();

        updateSCTScreen(tft, stepM.getStepCount());

        Serial.print("[SCT] Steps: ");
        Serial.println(stepM.getStepCount());
    }

    if (touched && tx >= 110 && tx <= 210 && ty >= 190 && ty <= 230) {
        goHome();
    }
}

void PaceID_Case() {
    static bool drawn = false;

    if (!drawn) {
        drawn = true;

        tft.fillScreen(APP_BG);

        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(APP_TEXT, APP_BG);
        tft.drawString("PACE ID TEST", 10, 10, 2);

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

        tft.fillRect(10, 50, 300, 120, APP_BG);

        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(APP_MUTED, APP_BG);
        tft.drawString("Current Pace:", 10, 50, 2);

        tft.setTextColor(APP_TEXT, APP_BG);
        tft.drawString(paceM.getPace(), 10, 80, 4);

        tft.setTextColor(APP_MUTED, APP_BG);
        tft.drawString("Steps:", 10, 150, 2);

        char buf[12];
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)stepM.getStepCount());

        tft.setTextColor(APP_TEXT, APP_BG);
        tft.drawString(buf, 10, 175, 2);
    }

    if (touched && tx >= 110 && tx <= 210 && ty >= 190 && ty <= 230) {
        drawn = false;
        goHome();
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
    calibM.begin();
    calibM.startCalibration();
    drawCalibrationScreen(tft);

    Serial.println("Boot: calibration started.");
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
    stepM.begin();

    // load saved profile - skip setup wizard if already done
    calorieM.begin();

    if (calorieM.isProfileSet()) {
        currentCase = CASE_CT;
        initCalibration();
    } else {
        currentCase = CASE_SETUP;
        setupStep   = SETUP_WELCOME;
        drawSetupWelcome(tft);
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

        case CASE_SCT:
            StepCountTest_Case(now);
            break;

        case CASE_PIDT:
            PaceID_Case();
            break;

        case CASE_SETTINGS:
            Settings_Case();
            break;
    }
}
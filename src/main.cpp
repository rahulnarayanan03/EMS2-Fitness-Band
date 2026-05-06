/* main.cpp
 State machine coordinator for the EMS2 fitness band.

 Cases:
   1 = C.T   - calibration, runs automatically on boot
   2 = HOME  - main homepage (UI.h/cpp), Red's sprite, steps, app buttons
   3 = S.T   - self test (stub, not yet implemented)
   4 = S.C.T - step count test (live counter + serial output)
   5 = P.ID.T - pace ID test (stub, not yet implemented)

 Boot flow:
   Power on -> Case 1 (calibration runs) -> home button appears -> Case 2

 From Case 2 the user taps app buttons to reach Cases 1/3/4/5.
 Every app case has a home button that returns to Case 2.
 Re-entering Case 1 saves current steps, re-calibrates, then asks
 whether to keep or reset the step count. */

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

// ---- hardware ---------------------------------------------------------------

TFT_eSPI tft = TFT_eSPI();

#define TOUCH_CS  33
#define TOUCH_IRQ 36
#define ST_PIN    4

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700;
uint16_t touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

// ---- module objects ---------------------------------------------------------

Calibration calibM;
StepCounter stepM(calibM);
PACEFIND    paceM;
SelfTest    stM(calibM, ST_PIN);
UI          ui(tft, stepM, calibM);

// ---- state machine ----------------------------------------------------------

enum AppCase { CASE_CT = 1, CASE_HOME = 2, CASE_ST = 3, CASE_SCT = 4, CASE_PIDT = 5 };
AppCase currentCase = CASE_CT;

bool     caseCtIsReentry     = false;
uint32_t savedStepsBeforeCal = 0;
bool     awaitingStepChoice  = false;

// touch state, populated once per loop tick and shared across case functions
uint16_t tx = 0, ty = 0;
bool     touched = false;

// timestamp of the last case transition - touched is ignored within 200ms of one
uint32_t lastTransition = 0;

// ---- helpers ----------------------------------------------------------------

bool readTouch(uint16_t &tx, uint16_t &ty) {
    if (!ts.touched()) return false;

    long sumX = 0, sumY = 0;
    int count = 0;
    for (int i = 0; i < 5; i++) {
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            sumX += p.x;
            sumY += p.y;
            count++;
        }
        delay(10);  // let the XPT2046 settle between samples, not debounce
    }
    if (count == 0) return false;

    tx = map(sumX / count, touchScreenMinimumX, touchScreenMaximumX, 0, tft.width());
    ty = map(sumY / count, touchScreenMinimumY, touchScreenMaximumY, 0, tft.height());
    return true;
}

// go to homepage and mark the transition time for debounce
void goHome() {
    currentCase    = CASE_HOME;
    lastTransition = millis();
    ui.begin();
}

// update step count and pace if calibration is done
void updateStepAndPace(uint32_t now) {
    if (!calibM.isCalibrated()) return;
    stepM.update();
    if (stepM.wasStepDetected()) paceM.update(now);
    else paceM.checkTimeout(now);
}

// ---- case functions ---------------------------------------------------------

void Calibration_Case() {
    calibM.update();

    static unsigned long lastTick = 0;
    if (!awaitingStepChoice && millis() - lastTick >= 1000) {
        lastTick = millis();
        tft.fillRect(0, 76, 240, 24, GB_LIGHTEST);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(GB_DARK, GB_LIGHTEST);
        tft.drawString("Sampling...", 10, 80, 2);  // pulse so user knows it's alive
    }

    if (!awaitingStepChoice && calibM.isCalibrated()) {
        awaitingStepChoice = true;
        drawCalibrationDone(tft, caseCtIsReentry, savedStepsBeforeCal);
        Serial.println("Calibration complete.");
    }

    if (awaitingStepChoice && touched) {
        if (caseCtIsReentry) {
            // KEEP (x 10-110, y 110-150)
            if (tx >= 10 && tx <= 110 && ty >= 110 && ty <= 150) {
                Serial.println("CT: keeping previous step count.");
                awaitingStepChoice = false;
                caseCtIsReentry    = false;
                goHome();
            }
            // RESET (x 130-230, y 110-150)
            else if (tx >= 130 && tx <= 230 && ty >= 110 && ty <= 150) {
                stepM.resetCount();
                Serial.println("CT: step count reset to 0.");
                awaitingStepChoice = false;
                caseCtIsReentry    = false;
                goHome();
            }
        } else {
            // first boot HOME button (x 70-170, y 140-180)
            if (tx >= 70 && tx <= 170 && ty >= 140 && ty <= 180) {
                awaitingStepChoice = false;
                goHome();
            }
        }
    }
}

void Home_Case(uint32_t now) {
    updateStepAndPace(now);

    // swap 0,0 for real RTC hour/minute once the PCB RTC is wired up
    ui.setTime(0, 0);
    ui.setDate(1, 1);
    ui.setPace(paceM.getPace());
    ui.update(now);

    if (touched) {
        uint8_t btnIndex = 0;
        if (ui.checkButtonTouch(tx, ty, btnIndex)) {
            lastTransition = millis();  // debounce any navigation away from home
            switch (btnIndex) {
                case 0: // C.T - save steps then re-calibrate
                    stepM.saveNow();
                    savedStepsBeforeCal = stepM.getStepCount();
                    caseCtIsReentry     = true;
                    awaitingStepChoice  = false;
                    currentCase         = CASE_CT;
                    calibM.startCalibration();
                    drawCalibrationScreen(tft);
                    Serial.println("Re-entering calibration.");
                    break;
                case 1: // S.T
                    currentCase = CASE_ST;
                    break;
                case 2: // S.C.T
                    currentCase = CASE_SCT;
                    drawSCTScreen(tft);
                    break;
                case 3: // P.ID.T
                    currentCase = CASE_PIDT;
                    break;
            }
        }
    }
}

void SelfTest_Case() {
    static bool stRan = false;
    if (!stRan) {
        stRan       = true;
        bool passed = stM.run();
        drawSelfTestScreen(tft, passed, stM.getResultStr(),
                           stM.getDeltaX(), stM.getDeltaY(), stM.getDeltaZ());
    }
    // home button (x 70-170, y 260-300)
    if (touched && tx >= 70 && tx <= 170 && ty >= 260 && ty <= 300) {
        stRan = false;  // reset so next visit reruns the test
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

    // home button (x 70-170, y 260-300)
    if (touched && tx >= 70 && tx <= 170 && ty >= 260 && ty <= 300) {
        goHome();
    }
}

void PaceID_Case() {
    static bool drawn = false;
    if (!drawn) {
        drawn = true;
        tft.fillScreen(GB_LIGHTEST);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
        tft.drawString("PACE ID TEST", 10, 10, 2);

        tft.fillRect(70, 260, 100, 40, GB_MID);
        tft.drawRect(70, 260, 100, 40, GB_DARKEST);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(GB_DARKEST, GB_MID);
        tft.drawString("HOME", 120, 280, 2);
    }

    // run step detection and pace update
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

        tft.fillRect(10, 50, 220, 120, GB_LIGHTEST);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(GB_DARK, GB_LIGHTEST);
        tft.drawString("Current Pace:", 10, 50, 2);
        tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
        tft.drawString(paceM.getPace(), 10, 80, 4);

        tft.setTextColor(GB_DARK, GB_LIGHTEST);
        tft.drawString("Steps:", 10, 150, 2);
        char buf[12];
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)stepM.getStepCount());
        tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
        tft.drawString(buf, 10, 175, 2);
    }

    // home button (x 70-170, y 260-300)
    if (touched && tx >= 70 && tx <= 170 && ty >= 260 && ty <= 300) {
        drawn = false;
        goHome();
    }
}

// ---- setup helpers ----------------------------------------------------------

void initDisplay() {
    tft.init();
    tft.setRotation(0);
    touchSPI.begin(25, 39, 32);
    SPI.begin(25, 39, 32);
    ts.begin();
    ts.setRotation(2);
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
    delay(2000);  // let serial monitor connect before any output

    stM.begin();
    initDisplay();
    stepM.begin();   // load saved step count from NVS before calibration starts
    initCalibration();
}

// ---- loop -------------------------------------------------------------------

void loop() {
    uint32_t now = millis();

    // suppress touches for 200ms after any case transition (debounce)
    touched = (millis() - lastTransition >= 200) && readTouch(tx, ty);

    switch (currentCase) {
        case CASE_CT:   Calibration_Case();      break;
        case CASE_HOME: Home_Case(now);           break;
        case CASE_ST:   SelfTest_Case();          break;
        case CASE_SCT:  StepCountTest_Case(now);  break;
        case CASE_PIDT: PaceID_Case();            break;
    }
}
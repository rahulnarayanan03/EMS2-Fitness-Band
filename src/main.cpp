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
#include "MAX30105.h"
#include "heartrate/heartrate.h"
#include "calibration/calibration.h"
#include "steptrack/step_counter.h"
#include "display/UI/UI.h"

// ---- hardware ---------------------------------------------------------------

TFT_eSPI tft = TFT_eSPI();

#define TOUCH_CS  33
#define TOUCH_IRQ 36

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700;
uint16_t touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

// ---- module objects ---------------------------------------------------------

MAX30105    particleSensor;
HeartRate   hrM;
Calibration calibM;
StepCounter stepM(calibM);
UI          ui(tft, stepM, calibM);

// ---- state machine ----------------------------------------------------------

enum AppCase { CASE_CT = 1, CASE_HOME = 2, CASE_ST = 3, CASE_SCT = 4, CASE_PIDT = 5 };
AppCase currentCase = CASE_CT;

// used when returning to C.T from homepage
bool     caseCtIsReentry     = false;
uint32_t savedStepsBeforeCal = 0;

// set true once calibration finishes so we wait for the user's button press
bool awaitingStepChoice = false;

// ---- touch helper -----------------------------------------------------------

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
        delay(10);
    }
    if (count == 0) return false;

    tx = map(sumX / count, touchScreenMinimumX, touchScreenMaximumX, 0, tft.width());
    ty = map(sumY / count, touchScreenMinimumY, touchScreenMaximumY, 0, tft.height());
    return true;
}

// ---- Case 1 screen helpers --------------------------------------------------

void drawCalibrationScreen() {
    tft.fillScreen(GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString("CALIBRATION", 10, 10, 2);
    tft.setTextColor(GB_DARK, GB_LIGHTEST);
    tft.drawString("Move the band slowly", 10, 40, 1);
    tft.drawString("in all directions.", 10, 54, 1);
    tft.drawString("Sampling...", 10, 80, 2);
}

void drawCalibrationDone() {
    tft.fillScreen(GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString("Calibration done!", 10, 10, 2);

    if (caseCtIsReentry) {
        // show previous step count and offer keep/reset
        tft.setTextColor(GB_DARK, GB_LIGHTEST);
        tft.drawString("Previous steps:", 10, 50, 1);

        char buf[16];
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)savedStepsBeforeCal);
        tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
        tft.drawString(buf, 10, 64, 2);

        tft.fillRect(10, 110, 100, 40, GB_MID);
        tft.drawRect(10, 110, 100, 40, GB_DARKEST);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(GB_DARKEST, GB_MID);
        tft.drawString("KEEP", 60, 130, 2);

        tft.fillRect(130, 110, 100, 40, GB_LIGHT);
        tft.drawRect(130, 110, 100, 40, GB_DARKEST);
        tft.setTextColor(GB_DARKEST, GB_LIGHT);
        tft.drawString("RESET", 180, 130, 2);
    } else {
        // first boot - just show home button
        tft.fillRect(70, 140, 100, 40, GB_MID);
        tft.drawRect(70, 140, 100, 40, GB_DARKEST);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(GB_DARKEST, GB_MID);
        tft.drawString("HOME", 120, 160, 2);
    }
}

// ---- Case 4: S.C.T screen helpers -------------------------------------------

void drawSCTScreen() {
    tft.fillScreen(GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString("STEP COUNT TEST", 10, 10, 2);
    tft.setTextColor(GB_DARK, GB_LIGHTEST);
    tft.drawString("Live output also on", 10, 40, 1);
    tft.drawString("Serial monitor.", 10, 54, 1);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString("STEPS", 10, 90, 1);

    tft.fillRect(70, 260, 100, 40, GB_MID);
    tft.drawRect(70, 260, 100, 40, GB_DARKEST);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(GB_DARKEST, GB_MID);
    tft.drawString("HOME", 120, 280, 2);
}

void updateSCTScreen() {
    tft.fillRect(10, 104, 220, 30, GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    char buf[12];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)stepM.getStepCount());
    tft.drawString(buf, 10, 106, 4);
}

// ---- Cases 3/5: stub screen -------------------------------------------------

void drawStubScreen(const char *title) {
    tft.fillScreen(GB_LIGHTEST);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(GB_DARKEST, GB_LIGHTEST);
    tft.drawString(title, 10, 10, 2);
    tft.setTextColor(GB_DARK, GB_LIGHTEST);
    tft.drawString("Not yet available.", 10, 40, 1);

    tft.fillRect(70, 260, 100, 40, GB_MID);
    tft.drawRect(70, 260, 100, 40, GB_DARKEST);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(GB_DARKEST, GB_MID);
    tft.drawString("HOME", 120, 280, 2);
}

// ---- setup ------------------------------------------------------------------

void setup() {
    Serial.begin(9600);
    delay(2000);

    tft.init();
    tft.setRotation(0);

    touchSPI.begin(25, 39, 32);
    SPI.begin(25, 39, 32);
    ts.begin();
    ts.setRotation(2);

    // load saved step count from NVS before calibration starts
    stepM.begin();

    Wire.begin(21, 22);
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("MAX30102 not found!");
    } else {
        particleSensor.setup();
        particleSensor.setPulseAmplitudeRed(0xFF);
        particleSensor.setPulseAmplitudeIR(0xFF);
        particleSensor.setPulseAmplitudeGreen(0);
        particleSensor.setSampleRate(400);
        particleSensor.setPulseWidth(411);
        particleSensor.setADCRange(16384);
        Serial.println("MAX30102 ready.");
    }

    // boot into Case 1 - calibration starts immediately
    calibM.begin();
    calibM.startCalibration();
    drawCalibrationScreen();
    Serial.println("Boot: calibration started.");
}

// ---- loop -------------------------------------------------------------------

void loop() {
    uint32_t now = millis();

    // heart rate runs in all cases
    long irValue = particleSensor.getIR();
    hrM.update(irValue);

    uint16_t tx = 0, ty = 0;
    bool touched = readTouch(tx, ty);

    switch (currentCase) {

        // ------------------------------------------------------------------ //
        case CASE_CT: {
            calibM.update();

            // pulse the "Sampling..." text so the user knows it's alive
            static unsigned long lastTick = 0;
            if (!awaitingStepChoice && millis() - lastTick >= 1000) {
                lastTick = millis();
                tft.fillRect(0, 76, 240, 24, GB_LIGHTEST);
                tft.setTextDatum(TL_DATUM);
                tft.setTextColor(GB_DARK, GB_LIGHTEST);
                tft.drawString("Sampling...", 10, 80, 2);
            }

            // calibration just finished - show the done screen
            if (!awaitingStepChoice && calibM.isCalibrated()) {
                awaitingStepChoice = true;
                drawCalibrationDone();
                Serial.println("Calibration complete.");
            }

            // wait for the user to tap a button on the done screen
            if (awaitingStepChoice && touched) {
                if (caseCtIsReentry) {
                    // KEEP (x 10-110, y 110-150)
                    if (tx >= 10 && tx <= 110 && ty >= 110 && ty <= 150) {
                        Serial.println("CT: keeping previous step count.");
                        awaitingStepChoice = false;
                        caseCtIsReentry    = false;
                        currentCase        = CASE_HOME;
                        ui.begin();
                        delay(200);
                    }
                    // RESET (x 130-230, y 110-150)
                    else if (tx >= 130 && tx <= 230 && ty >= 110 && ty <= 150) {
                        stepM.resetCount();
                        Serial.println("CT: step count reset to 0.");
                        awaitingStepChoice = false;
                        caseCtIsReentry    = false;
                        currentCase        = CASE_HOME;
                        ui.begin();
                        delay(200);
                    }
                } else {
                    // first boot HOME button (x 70-170, y 140-180)
                    if (tx >= 70 && tx <= 170 && ty >= 140 && ty <= 180) {
                        awaitingStepChoice = false;
                        currentCase        = CASE_HOME;
                        ui.begin();
                        delay(200);
                    }
                }
            }
            break;
        }

        // ------------------------------------------------------------------ //
        case CASE_HOME: {
            if (calibM.isCalibrated()) stepM.update();

            // swap 0,0 for real RTC hour/minute once the PCB RTC is wired up
            ui.setTime(0, 0);
            ui.setDate(1, 1);
            ui.setBPM(hrM.getBPM());
            ui.update(now);

            if (touched) {
                uint8_t btnIndex = 0;
                if (ui.checkButtonTouch(tx, ty, btnIndex)) {
                    delay(200);
                    switch (btnIndex) {
                        case 0: // C.T - save steps then re-calibrate
                            stepM.saveNow();
                            savedStepsBeforeCal = stepM.getStepCount();
                            caseCtIsReentry     = true;
                            awaitingStepChoice  = false;
                            currentCase         = CASE_CT;
                            calibM.startCalibration();
                            drawCalibrationScreen();
                            Serial.println("Re-entering calibration.");
                            break;
                        case 1: // S.T stub
                            currentCase = CASE_ST;
                            drawStubScreen("SELF TEST");
                            break;
                        case 2: // S.C.T
                            currentCase = CASE_SCT;
                            drawSCTScreen();
                            break;
                        case 3: // P.ID.T stub
                            currentCase = CASE_PIDT;
                            drawStubScreen("PACE ID TEST");
                            break;
                    }
                }
            }
            break;
        }

        // ------------------------------------------------------------------ //
        case CASE_ST: {
            // home button (x 70-170, y 260-300)
            if (touched && tx >= 70 && tx <= 170 && ty >= 260 && ty <= 300) {
                currentCase = CASE_HOME;
                ui.begin();
                delay(200);
            }
            break;
        }

        // ------------------------------------------------------------------ //
        case CASE_SCT: {
            if (calibM.isCalibrated()) stepM.update();

            static unsigned long lastSCTUpdate = 0;
            if (millis() - lastSCTUpdate >= 500) {
                lastSCTUpdate = millis();
                updateSCTScreen();
                Serial.print("[SCT] Steps: ");
                Serial.println(stepM.getStepCount());
            }

            // home button (x 70-170, y 260-300)
            if (touched && tx >= 70 && tx <= 170 && ty >= 260 && ty <= 300) {
                currentCase = CASE_HOME;
                ui.begin();
                delay(200);
            }
            break;
        }

        // ------------------------------------------------------------------ //
        case CASE_PIDT: {
            // home button (x 70-170, y 260-300)
            if (touched && tx >= 70 && tx <= 170 && ty >= 260 && ty <= 300) {
                currentCase = CASE_HOME;
                ui.begin();
                delay(200);
            }
            break;
        }
    }
}
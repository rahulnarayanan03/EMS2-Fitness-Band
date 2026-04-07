#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include "MAX30105.h"
#include "heartrate/heartrate.h"
#include "calibration/calibration.h"

TFT_eSPI tft = TFT_eSPI();

#define TOUCH_CS  33
#define TOUCH_IRQ 36

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700;
uint16_t touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

enum AppState { HOME, CALIBRATING, CALIBRATED };
AppState currentState = HOME;

MAX30105 particleSensor;
HeartRate hrM;
Calibration calibM;

void drawHome(int bpm) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("EMS2 Testing");

    tft.setCursor(10, 50);
    tft.println("Heart Rate:");
    tft.setTextSize(3);
    tft.setCursor(10, 80);
    tft.print(bpm);
    tft.println(" BPM");

    tft.setTextSize(2);

    tft.fillRect(60, 180, 120, 50, TFT_BLUE);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(65, 198);
    tft.println("Calibrate");
}

void drawCalibratingScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("Calibrating...");
    tft.setCursor(10, 40);
    tft.println("Move sensor");
    tft.setCursor(10, 65);
    tft.println("all directions!");
}

void drawDataScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("Live Data");

    tft.fillRect(60, 260, 120, 40, TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(85, 272);
    tft.println("Home");
}

void updateDataScreen() {
    float x = calibM.getXG();
    float y = calibM.getYG();
    float z = calibM.getZG();
    float mag = sqrt(x*x + y*y + z*z);

    tft.fillRect(0, 40, 240, 200, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);

    tft.setCursor(10, 50);
    tft.print("X: "); tft.println(x, 3);

    tft.setCursor(10, 80);
    tft.print("Y: "); tft.println(y, 3);

    tft.setCursor(10, 110);
    tft.print("Z: "); tft.println(z, 3);

    tft.setCursor(10, 150);
    tft.print("Mag: "); tft.println(mag, 3);
}

void setup() {
    Serial.begin(9600);
    delay(2000);

    tft.init();
    tft.setRotation(0);

    touchSPI.begin(25, 39, 32);
    SPI.begin(25, 39, 32);
    ts.begin();
    ts.setRotation(2);

    calibM.begin();

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

    drawHome(0);
    Serial.println("Ready.");
}

void loop() {

    long irValue = particleSensor.getIR();
    hrM.update(irValue);

    if (currentState == HOME) {
        static unsigned long lastBPMUpdate = 0;
        if (millis() - lastBPMUpdate >= 1000) {
            lastBPMUpdate = millis();
            drawHome(hrM.getBPM());
            Serial.print("BPM: "); Serial.println(hrM.getBPM());
            Serial.print("IR: "); Serial.println(irValue);
        }
    }

    if (currentState == CALIBRATING) {
        calibM.update();

        static unsigned long lastTick = 0;
        if (millis() - lastTick >= 1000) {
            lastTick = millis();
            tft.fillRect(0, 90, 240, 30, TFT_BLACK);
            tft.setTextColor(TFT_YELLOW);
            tft.setTextSize(2);
            tft.setCursor(10, 90);
            tft.println("Sampling...");
        }

        if (calibM.isCalibrated()) {
            currentState = CALIBRATED;
            drawDataScreen();
        }
    }

    if (currentState == CALIBRATED) {
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate >= 500) {
            lastUpdate = millis();
            updateDataScreen();
        }
    }

    if (ts.touched()) {
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
        if (count == 0) return;

        uint16_t x = map(sumX / count, touchScreenMinimumX, touchScreenMaximumX, 0, tft.width());
        uint16_t y = map(sumY / count, touchScreenMinimumY, touchScreenMaximumY, 0, tft.height());

        Serial.print("Touch: "); Serial.print(x);
        Serial.print(", "); Serial.println(y);

        if (currentState == HOME) {
            if (x >= 60 && x <= 180 && y >= 180 && y <= 230) {
                currentState = CALIBRATING;
                calibM.startCalibration();
                drawCalibratingScreen();
            }
        } else if (currentState == CALIBRATED) {
            if (x >= 60 && x <= 180 && y >= 260 && y <= 300) {
                currentState = HOME;
                drawHome(hrM.getBPM());
            }
        }

        delay(200);
    }
}
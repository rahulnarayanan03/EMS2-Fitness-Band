#include <Arduino.h>
#include "Calibration.h"

const int xPin = 26;
const int yPin = 35;
const int zPin = 34;

const int NUM_SAMPLES = 32;

float Calibration::readVoltage(int pin) {
    long sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(pin);
        delayMicroseconds(100);
    }
    return (sum / (float)NUM_SAMPLES) * (3.3f / 4095.0f);
}

void Calibration::begin() {
    calibrating = false;
    calibrated = false;

    pinMode(xPin, INPUT);
    pinMode(yPin, INPUT);
    pinMode(zPin, INPUT);

    analogSetAttenuation(ADC_11db);
}

void Calibration::startCalibration() {
    calibrating = true;
    calibrated = false;
    startTime = millis();

    xMin = yMin = zMin = 9999;
    xMax = yMax = zMax = -9999;

    Serial.println("Calibration started...");
    Serial.println("Move sensor in ALL directions!");
}

void Calibration::update() {

    if (calibrating) {

        float voltageX = readVoltage(xPin);
        float voltageY = readVoltage(yPin);
        float voltageZ = readVoltage(zPin);

        if (voltageX < xMin) xMin = voltageX;
        if (voltageX > xMax) xMax = voltageX;

        if (voltageY < yMin) yMin = voltageY;
        if (voltageY > yMax) yMax = voltageY;

        if (voltageZ < zMin) zMin = voltageZ;
        if (voltageZ > zMax) zMax = voltageZ;

        Serial.print("X: "); Serial.print(voltageX);
        Serial.print(" | Y: "); Serial.print(voltageY);
        Serial.print(" | Z: "); Serial.println(voltageZ);

        if (millis() - startTime > 20000) {

            xOffset = (xMax + xMin) / 2.0f;
            yOffset = (yMax + yMin) / 2.0f;
            zOffset = (zMax + zMin) / 2.0f;

            xScale = (xMax - xMin) / 2.0f;
            yScale = (yMax - yMin) / 2.0f;
            zScale = (zMax - zMin) / 2.0f;

            // ADC2 on GPIO26 has limited voltage swing
            // clamp xScale to minimum so getXG() doesn't produce crazy values
            if (xScale < 0.05f) xScale = 0.05f;

            calibrating = false;
            calibrated = true;

            Serial.println("----- CALIBRATION COMPLETE -----");
            Serial.print("X Min: "); Serial.println(xMin);
            Serial.print("X Max: "); Serial.println(xMax);
            Serial.print("Y Min: "); Serial.println(yMin);
            Serial.print("Y Max: "); Serial.println(yMax);
            Serial.print("Z Min: "); Serial.println(zMin);
            Serial.print("Z Max: "); Serial.println(zMax);
            Serial.println("---- OFFSETS & SCALES ----");
            Serial.print("X Offset: "); Serial.print(xOffset); Serial.print("  Scale: "); Serial.println(xScale);
            Serial.print("Y Offset: "); Serial.print(yOffset); Serial.print("  Scale: "); Serial.println(yScale);
            Serial.print("Z Offset: "); Serial.print(zOffset); Serial.print("  Scale: "); Serial.println(zScale);
        }
    }
}

bool Calibration::isCalibrated() { return calibrated; }

float Calibration::getXOffset() { return xOffset; }
float Calibration::getYOffset() { return yOffset; }
float Calibration::getZOffset() { return zOffset; }

float Calibration::getXG() { return (readVoltage(xPin) - xOffset) / xScale; }
float Calibration::getYG() { return (readVoltage(yPin) - yOffset) / yScale; }
float Calibration::getZG() { return (readVoltage(zPin) - zOffset) / zScale; }
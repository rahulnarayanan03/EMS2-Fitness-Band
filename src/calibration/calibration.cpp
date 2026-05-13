#include "calibration.h"

const int xPin = 34;
const int yPin = 35;
const int zPin = 26;

// labels for each of the 6 directions shown on screen
const char* Calibration::DIR_LABEL[6] = {
    "Point X+ UP",
    "Point X- UP",
    "Point Y+ UP",
    "Point Y- UP",
    "Point Z+ UP",
    "Point Z- UP"
};

// average a bunch of ADC reads to smooth out noise
float Calibration::readVoltage(int pin) {
    long sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(pin);
        delayMicroseconds(100);
    }
    return (sum / (float)NUM_SAMPLES) * (3.3f / 4095.0f);
}

void Calibration::begin() {
    pinMode(xPin, INPUT);
    pinMode(yPin, INPUT);
    pinMode(zPin, INPUT);
    analogSetAttenuation(ADC_11db);

    stage      = Stage::IDLE;
    calibrated = false;
}

void Calibration::startCalibration() {
    dirIndex   = 0;
    stage      = Stage::PREP;
    calibrated = false;
    stageStart = millis();

    memset(means,  0, sizeof(means));
    memset(dirSum, 0, sizeof(dirSum));
    dirCount = 0;

    Serial.println("[CAL] Starting guided calibration, 6 directions.");
    Serial.printf("[CAL]  Direction 1: %s\n", DIR_LABEL[0]);
}

void Calibration::update() {
    if (stage == Stage::IDLE || stage == Stage::DONE) return;

    uint32_t elapsed = millis() - stageStart;

    if (stage == Stage::PREP) {
        // just waiting for the user to get into position
        if (elapsed >= PREP_MS) {
            stage      = Stage::SAMPLING;
            stageStart = millis();
            dirSum[0] = dirSum[1] = dirSum[2] = 0;
            dirCount  = 0;
            Serial.printf("[CAL] Sampling dir %d: %s\n", dirIndex + 1, DIR_LABEL[dirIndex]);
        }

    } else if (stage == Stage::SAMPLING) {
        // keep accumulating readings while the user holds still
        dirSum[0] += readVoltage(xPin);
        dirSum[1] += readVoltage(yPin);
        dirSum[2] += readVoltage(zPin);
        dirCount++;

        if (elapsed >= SAMPLE_MS) {
            // save the mean for this direction then move on
            means[dirIndex][0] = dirSum[0] / dirCount;
            means[dirIndex][1] = dirSum[1] / dirCount;
            means[dirIndex][2] = dirSum[2] / dirCount;

            Serial.printf("[CAL] Dir %d done  X=%.3f Y=%.3f Z=%.3f  (n=%d)\n",
                          dirIndex + 1,
                          means[dirIndex][0],
                          means[dirIndex][1],
                          means[dirIndex][2],
                          dirCount);

            dirIndex++;

            if (dirIndex >= NUM_DIRECTIONS) {
                finalise();
            } else {
                // prep window for next direction
                stage      = Stage::PREP;
                stageStart = millis();
                memset(dirSum, 0, sizeof(dirSum));
                dirCount = 0;
                Serial.printf("[CAL] Next: %s\n", DIR_LABEL[dirIndex]);
            }
        }
    }
}

bool               Calibration::isCalibrated() { return calibrated; }
Calibration::Stage Calibration::getStage()     { return stage; }
int                Calibration::getDirIndex()   { return dirIndex; }

int Calibration::getSecsLeft() {
    uint32_t elapsed = millis() - stageStart;
    float    total   = (stage == Stage::PREP) ? PREP_MS : SAMPLE_MS;
    int      left    = (int)((total - elapsed) / 1000) + 1;
    return max(0, left);
}

float Calibration::getXOffset() { return xOffset; }
float Calibration::getYOffset() { return yOffset; }
float Calibration::getZOffset() { return zOffset; }
float Calibration::getXG()      { return (readVoltage(xPin) - xOffset) / xScale; }
float Calibration::getYG()      { return (readVoltage(yPin) - yOffset) / yScale; }
float Calibration::getZG()      { return (readVoltage(zPin) - zOffset) / zScale; }

float Calibration::getX_mV() {
    return 1000.0f * (readVoltage(xPin) - 1.35);
}

float Calibration::getY_mV() {
    return 1000.0f * (readVoltage(yPin));
}

float Calibration::getZ_mV() {
    return 1000.0f * (readVoltage(zPin));
}

void Calibration::finalise() {
    // find the min/max across all 6 direction means for each axis
    float xMin =  9999, xMax = -9999;
    float yMin =  9999, yMax = -9999;
    float zMin =  9999, zMax = -9999;

    for (int d = 0; d < NUM_DIRECTIONS; d++) {
        if (means[d][0] < xMin) xMin = means[d][0];
        if (means[d][0] > xMax) xMax = means[d][0];
        if (means[d][1] < yMin) yMin = means[d][1];
        if (means[d][1] > yMax) yMax = means[d][1];
        if (means[d][2] < zMin) zMin = means[d][2];
        if (means[d][2] > zMax) zMax = means[d][2];
    }

    // midpoint = offset, half-range = scale
    xOffset = (xMax + xMin) / 2.0f;
    yOffset = (yMax + yMin) / 2.0f;
    zOffset = (zMax + zMin) / 2.0f;

    // clamp scale so getXG/Y/Z don't blow up on dodgy ADC pins
    xScale = max((xMax - xMin) / 2.0f, SCALE_FLOOR);
    yScale = max((yMax - yMin) / 2.0f, SCALE_FLOOR);
    zScale = max((zMax - zMin) / 2.0f, SCALE_FLOOR);

    calibrated = true;
    stage      = Stage::DONE;

    Serial.println("[CAL] ---- COMPLETE ----");
    Serial.printf("[CAL] X  min=%.3f max=%.3f  offset=%.3f scale=%.3f\n", xMin, xMax, xOffset, xScale);
    Serial.printf("[CAL] Y  min=%.3f max=%.3f  offset=%.3f scale=%.3f\n", yMin, yMax, yOffset, yScale);
    Serial.printf("[CAL] Z  min=%.3f max=%.3f  offset=%.3f scale=%.3f\n", zMin, zMax, zOffset, zScale);
}
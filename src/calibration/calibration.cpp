#include "calibration.h"
#include <Preferences.h>

const int xPin = 34;
const int yPin = 35;
const int zPin = 26;

static constexpr char     CAL_NVS_NAMESPACE[] = "calib";
static constexpr uint32_t CAL_MAGIC           = 0xCA11B123;
static constexpr uint32_t CAL_VERSION         = 1;

const char* Calibration::DIR_LABEL[6] = {
    "Point X+ UP",
    "Point X- UP",
    "Point Y+ UP",
    "Point Y- UP",
    "Point Z+ UP",
    "Point Z- UP"
};

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

    stage = Stage::IDLE;

    if (loadFromNVS()) {
        calibrated = true;
        stage = Stage::DONE;

        Serial.println("[CAL] Loaded calibration from NVS.");
        Serial.printf("[CAL] X offset=%.3f scale=%.3f\n", xOffset, xScale);
        Serial.printf("[CAL] Y offset=%.3f scale=%.3f\n", yOffset, yScale);
        Serial.printf("[CAL] Z offset=%.3f scale=%.3f\n", zOffset, zScale);
    } else {
        calibrated = false;
        Serial.println("[CAL] No saved calibration found. Manual calibration required.");
    }
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
        if (elapsed >= PREP_MS) {
            stage      = Stage::SAMPLING;
            stageStart = millis();

            dirSum[0] = dirSum[1] = dirSum[2] = 0;
            dirCount  = 0;

            Serial.printf("[CAL] Sampling dir %d: %s\n", dirIndex + 1, DIR_LABEL[dirIndex]);
        }

    } else if (stage == Stage::SAMPLING) {
        dirSum[0] += readVoltage(xPin);
        dirSum[1] += readVoltage(yPin);
        dirSum[2] += readVoltage(zPin);
        dirCount++;

        if (elapsed >= SAMPLE_MS) {
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
                stage      = Stage::PREP;
                stageStart = millis();

                memset(dirSum, 0, sizeof(dirSum));
                dirCount = 0;

                Serial.printf("[CAL] Next: %s\n", DIR_LABEL[dirIndex]);
            }
        }
    }
}

bool Calibration::isCalibrated() {
    return calibrated;
}

Calibration::Stage Calibration::getStage() {
    return stage;
}

int Calibration::getDirIndex() {
    return dirIndex;
}

int Calibration::getSecsLeft() {
    uint32_t elapsed = millis() - stageStart;
    float total = (stage == Stage::PREP) ? PREP_MS : SAMPLE_MS;
    int left = (int)((total - elapsed) / 1000) + 1;

    return max(0, left);
}

float Calibration::getXOffset() {
    return xOffset;
}

float Calibration::getYOffset() {
    return yOffset;
}

float Calibration::getZOffset() {
    return zOffset;
}

float Calibration::getXG() {
    return (readVoltage(xPin) - xOffset) / xScale;
}

float Calibration::getYG() {
    return (readVoltage(yPin) - yOffset) / yScale;
}

float Calibration::getZG() {
    return (readVoltage(zPin) - zOffset) / zScale;
}

float Calibration::getX_mV() {
    return analogReadMilliVolts(xPin);
}

float Calibration::getY_mV() {
    return analogReadMilliVolts(yPin);
}

float Calibration::getZ_mV() {
    return analogReadMilliVolts(zPin);
}

void Calibration::finalise() {
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

    xOffset = (xMax + xMin) / 2.0f;
    yOffset = (yMax + yMin) / 2.0f;
    zOffset = (zMax + zMin) / 2.0f;

    xScale = max((xMax - xMin) / 2.0f, SCALE_FLOOR);
    yScale = max((yMax - yMin) / 2.0f, SCALE_FLOOR);
    zScale = max((zMax - zMin) / 2.0f, SCALE_FLOOR);

    calibrated = true;
    stage      = Stage::DONE;

    Serial.println("[CAL] ---- COMPLETE ----");
    Serial.printf("[CAL] X  min=%.3f max=%.3f  offset=%.3f scale=%.3f\n", xMin, xMax, xOffset, xScale);
    Serial.printf("[CAL] Y  min=%.3f max=%.3f  offset=%.3f scale=%.3f\n", yMin, yMax, yOffset, yScale);
    Serial.printf("[CAL] Z  min=%.3f max=%.3f  offset=%.3f scale=%.3f\n", zMin, zMax, zOffset, zScale);

    if (saveToNVS()) {
        Serial.println("[CAL] Calibration saved to NVS.");
    } else {
        Serial.println("[CAL] ERROR: Calibration could not be saved to NVS.");
    }
}

bool Calibration::loadFromNVS() {
    Preferences prefs;

    if (!prefs.begin(CAL_NVS_NAMESPACE, true)) {
        return false;
    }

    uint32_t magic = prefs.getUInt("magic", 0);
    uint32_t ver   = prefs.getUInt("ver", 0);

    if (magic != CAL_MAGIC || ver != CAL_VERSION) {
        prefs.end();
        return false;
    }

    xOffset = prefs.getFloat("xo", 0.0f);
    yOffset = prefs.getFloat("yo", 0.0f);
    zOffset = prefs.getFloat("zo", 0.0f);

    xScale = prefs.getFloat("xs", 0.0f);
    yScale = prefs.getFloat("ys", 0.0f);
    zScale = prefs.getFloat("zs", 0.0f);

    prefs.end();

    if (!isfinite(xOffset) || !isfinite(yOffset) || !isfinite(zOffset)) return false;
    if (!isfinite(xScale)  || !isfinite(yScale)  || !isfinite(zScale))  return false;

    if (xScale < SCALE_FLOOR || yScale < SCALE_FLOOR || zScale < SCALE_FLOOR) {
        return false;
    }

    return true;
}

bool Calibration::saveToNVS() {
    Preferences prefs;

    if (!prefs.begin(CAL_NVS_NAMESPACE, false)) {
        return false;
    }

    prefs.putUInt("magic", CAL_MAGIC);
    prefs.putUInt("ver", CAL_VERSION);

    prefs.putFloat("xo", xOffset);
    prefs.putFloat("yo", yOffset);
    prefs.putFloat("zo", zOffset);

    prefs.putFloat("xs", xScale);
    prefs.putFloat("ys", yScale);
    prefs.putFloat("zs", zScale);

    prefs.end();

    return true;
}
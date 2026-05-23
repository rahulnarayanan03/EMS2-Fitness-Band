#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>

class Calibration {
public:
    enum class Stage { IDLE, PREP, SAMPLING, DONE };

    static const char* DIR_LABEL[6];

    void  begin();
    void  startCalibration();
    void  update();

    bool  isCalibrated();
    Stage getStage();
    int   getDirIndex();
    int   getSecsLeft();

    float getXOffset();
    float getYOffset();
    float getZOffset();

    float getXG();
    float getYG();
    float getZG();

    float getX_mV();
    float getY_mV();
    float getZ_mV();

private:
    static constexpr int   NUM_DIRECTIONS = 6;
    static constexpr int   NUM_SAMPLES    = 32;
    static constexpr float SCALE_FLOOR    = 0.05f;
    static constexpr float PREP_MS        = 4000;
    static constexpr float SAMPLE_MS      = 2000;

    int        dirIndex   = 0;
    Stage      stage      = Stage::IDLE;
    bool       calibrated = false;
    uint32_t   stageStart = 0;

    float      dirSum[3] = {};
    int        dirCount  = 0;

    float      means[NUM_DIRECTIONS][3] = {};

    float xOffset = 0, yOffset = 0, zOffset = 0;
    float xScale  = 1, yScale  = 1, zScale  = 1;

    float readVoltage(int pin);
    void  finalise();

    bool  loadFromNVS();
    bool  saveToNVS();
};

#endif
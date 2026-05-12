#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>

class Calibration {
public:
    enum class Stage { IDLE, PREP, SAMPLING, DONE };

    static const char* DIR_LABEL[6];  // "Point X+ UP" etc, readable by main/screens

    void  begin();
    void  startCalibration();
    void  update();

    bool  isCalibrated();
    Stage getStage();
    int   getDirIndex();    // 0-5, which direction we're currently on
    int   getSecsLeft();    // countdown seconds left in current stage

    float getXOffset();
    float getYOffset();
    float getZOffset();

    float getXG();
    float getYG();
    float getZG();

    float getRawX();    // Get raw ADXL acceleration from the x-axis
    float getRawY();    // Get raw ADXL acceleration from the y-axis
    float getRawZ();    // Get raw ADXL acceleration from the z-axis

private:
    static constexpr int   NUM_DIRECTIONS = 6;
    static constexpr int   NUM_SAMPLES    = 32;
    static constexpr float SCALE_FLOOR    = 0.05f;
    static constexpr float PREP_MS        = 2000;
    static constexpr float SAMPLE_MS      = 4000;

    int        dirIndex  = 0;
    Stage      stage     = Stage::IDLE;
    bool       calibrated = false;
    uint32_t   stageStart = 0;

    float      dirSum[3] = {};
    int        dirCount  = 0;

    float      means[NUM_DIRECTIONS][3] = {};

    float xOffset = 0, yOffset = 0, zOffset = 0;
    float xScale  = 1, yScale  = 1, zScale  = 1;

    float readVoltage(int pin);
    void  finalise();
};

#endif
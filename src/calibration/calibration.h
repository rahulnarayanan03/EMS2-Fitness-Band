#ifndef CALIBRATION_H
#define CALIBRATION_H

class Calibration {
public:

    void begin();
    void startCalibration();
    void update();

    bool isCalibrated();

    float getXOffset();
    float getYOffset();
    float getZOffset();

    float getXG();
    float getYG();
    float getZG();

private:
    bool calibrating = false;
    bool calibrated = false;
    unsigned long startTime = 0;

    float xOffset = 0;
    float yOffset = 0;
    float zOffset = 0;

    float xScale = 1;
    float yScale = 1;
    float zScale = 1;

    float xMin, xMax;
    float yMin, yMax;
    float zMin, zMax;

    float readVoltage(int pin);
};

#endif
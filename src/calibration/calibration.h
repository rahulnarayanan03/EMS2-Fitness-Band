#ifndef CALIBRATION_H
#define CALIBRATION_H

class Calibration {
public:

    void begin();
    void startCalibration();
    void update();

    float getXOffset();
    float getYOffset();
    float getZOffset();

private:
    bool calibrating = false;
    unsigned long startTime = 0;

    // offsets
    float xOffset = 0;
    float yOffset = 0;
    float zOffset = 0;
    
};

#endif
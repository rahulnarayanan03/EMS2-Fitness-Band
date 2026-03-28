#include <Arduino.h>
#include "Calibration.h"

//temp pin assignments DONT FORGET TO CHANGE
const int xPin = 34;
const int yPin = 35;
const int zPin = 32;

//setup
void Calibration::begin() {
    calibrating = false;

    pinMode(xPin, INPUT);
    pinMode(yPin, INPUT);
    pinMode(zPin, INPUT);
}

//reigger fr calibration
void Calibration::startCalibration() {
    calibrating = true;
    startTime = millis();

    Serial.println("Calibration initialised - Testing");
}



void Calibration::update() {

    if (calibrating) {

        // wait 2 seconds for stability
        if (millis() - startTime > 2000) {

            // read raw values from ADXL335
            int rawX = analogRead(xPin);
            int rawY = analogRead(yPin);
            int rawZ = analogRead(zPin);

            //convert to voltage
            float voltageX = rawX * (3.3 / 4095.0);
            float voltageY = rawY * (3.3 / 4095.0);
            float voltageZ = rawZ * (3.3 / 4095.0);

            //acceleration conversiom
            float x = (voltageX - 1.65) / 0.33;
            float y = (voltageY - 1.65) / 0.33;
            float z = (voltageZ - 1.65) / 0.33;



            //offsets
            xOffset = x;
            yOffset = y;
            zOffset = z;

            //serial monitor
            Serial.println("Calibration Complete!");
            Serial.print("X Offset: "); 
            Serial.println(xOffset);
            Serial.print("Y Offset: "); 
            Serial.println(yOffset);
            Serial.print("Z Offset: "); 
            Serial.println(zOffset);

            calibrating = false;  // stop calibration
        }
    }
}

// getters
float Calibration::getXOffset() {
    return xOffset;

}

float Calibration::getYOffset() {
    return yOffset;

}

float Calibration::getZOffset() {
    return zOffset;

}
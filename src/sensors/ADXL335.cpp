#include "ADXL335.h"
#include <Arduino.h>

ADXL335::ADXL335(int pinX, int pinY, int pinZ) {
    _pinX = pinX;
    _pinY = pinY;
    _pinZ = pinZ;

}

void ADXL335::begin() {
    
}

void ADXL335::readAcc(float &x, float &y, float &z) {
    x = analogRead(_pinX);
    y = analogRead(_pinY);
    z = analogRead(_pinZ);
}
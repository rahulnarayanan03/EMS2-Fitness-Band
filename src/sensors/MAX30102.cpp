#include "MAX30102.h"

bool MAX30102::begin() {
    Wire.begin();

    if (!heartS.begin(Wire)) {
        return false;
    }

    //Configure sensor with default safe settings
    heartS.setup();

    return true;
}

long MAX30102::readIR() {
    //Returning raw IR value from the heartrate
    return heartS.getIR();
}
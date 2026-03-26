#ifndef MAX30102_H
#define MAX30102_H

#include <Wire.h>
#include "MAX30105.h"

class MAX30102 {
public:
    bool begin();

    //Reading raw IR value via pulse oximeter 
    long readIR();

private:
    
    //NOTE: This model MAX30105 is apparently used for our model version too.
    MAX30105 heartS;
};

#endif
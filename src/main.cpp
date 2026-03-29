#include <Arduino.h>

#include "sensors/MAX30102.h"
#include "sensors/ADXL335.h"

#include "heartrate/heartrate.h"
#include "pacefind/pacefind.h"
#include "display/display.h"
#include "calibration/calibration.h"
//add rest

// create objects
MAX30102 heartSM;
HeartRate hrM;
PACEFIND paceM;
Calibration calibM;
Display screenM;
ADXL335 adxlM(32, 33, 34); //Pins DO NOT FORGET TO CHANGE FOR TESTING

void setup() {

    Serial.begin(9600);
    delay(2000);

    Serial.println("Compile test");

    //basic initialisation
    screenM.begin();
    calibM.begin();

    Serial.println("Everything is compiled");

    calibM.startCalibration();
}

void loop() {

    calibM.update();

    Serial.println("Testing Testing Hello World");
    delay(1000);
}
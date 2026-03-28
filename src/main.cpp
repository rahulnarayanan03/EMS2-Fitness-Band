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
//ADXL335 adxlM;

void setup() {

    Serial.begin(115200);
    delay(2000);

    Serial.println("Compile test");

    //basic initialisation
    screenM.begin();
    calibM.begin();

    Serial.println("Everything is compiled");
}

void loop() {

    Serial.println("Testing Testing Hello World");
    delay(1000);
}
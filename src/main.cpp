#include <Arduino.h>
#include <math.h>

#include "sensors/MAX30102.h"
#include "sensors/ADXL335.h"

#include "heartrate/heartrate.h"
#include "pacefind/pacefind.h"
#include "display/display.h"
#include "calibration/calibration.h"

MAX30102 heartSM;
HeartRate hrM;
PACEFIND paceM;
Calibration calibM;
Display screenM;
ADXL335 adxlM(32, 35, 34);

void setup() {
    Serial.begin(9600);
    delay(2000);

    Serial.println("Compile test");

    screenM.begin();
    calibM.begin();

    Serial.println("Everything is compiled");

    calibM.startCalibration();
}

void loop() {
    calibM.update();

    if (calibM.isCalibrated()) {
        float x = calibM.getXG();
        float y = calibM.getYG();
        float z = calibM.getZG();

        float magnitude = sqrt(x*x + y*y + z*z);

        Serial.print("X: "); Serial.print(x, 3);
        Serial.print(" Y: "); Serial.print(y, 3);
        Serial.print(" Z: "); Serial.print(z, 3);
        Serial.print(" | Mag: "); Serial.println(magnitude, 3);

        // delay(250);
    }
}
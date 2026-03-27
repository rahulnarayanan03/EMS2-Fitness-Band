#include "PACEFIND.h"

void PACEFIND::update(unsigned long currentTime) {

    if (lastStepTime != 0) {
        unsigned long interval = currentTime - lastStepTime;

        //example implemntation for now, update later where needed
        if (interval < 400) {
            pace = "RUNNING";
        }
        else if (interval < 800) {
            pace = "WALKING";
        }
        else {
            pace = "STANDING";
        }
    }

    lastStepTime = currentTime;
}
//pace
const char* PACEFIND::getPace() {
    return pace;
}
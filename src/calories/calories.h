#ifndef CALORIES_H
#define CALORIES_H

#include <Arduino.h>
#include <Preferences.h>

class Calories {
public:
    void  begin();
    void  reset();
    void  onStep(const char* pace);

    float getKcal();

    void  saveProfile(int age, float weightKg, float heightCm);
    bool  isProfileSet();

    int   getAge();
    float getWeightKg();
    float getHeightCm();

private:
    bool  profileSet  = false;
    int   age         = 25;
    float weightKg    = 70.0f;
    float heightCm    = 170.0f;
    float totalKcal   = 0.0f;
    float stepsPerMin = 100.0f;

    uint32_t lastSaveMs = 0;

    void  saveKcal();
    float ageFitnessMultiplier();
    float metFromSpeed(float speedMps);
};

#endif
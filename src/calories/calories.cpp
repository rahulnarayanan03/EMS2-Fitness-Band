#include "calories.h"

void Calories::begin() {
    prefs.begin("calories", true);
    profileSet = prefs.getBool("set",   false);
    age        = prefs.getInt("age",    25);
    weightKg   = prefs.getFloat("wkg", 70.0f);
    heightCm   = prefs.getFloat("hcm", 170.0f);
    prefs.end();

    totalKcal = 0.0f;

    Serial.printf("[Calories] Profile loaded — set=%d age=%d weight=%.1f height=%.1f\n",
                  profileSet, age, weightKg, heightCm);
}

void Calories::saveProfile(int a, float w, float h) {
    age      = a;
    weightKg = w;
    heightCm = h;
    profileSet = true;

    prefs.begin("calories", false);
    prefs.putBool("set",    true);
    prefs.putInt("age",     age);
    prefs.putFloat("wkg",   weightKg);
    prefs.putFloat("hcm",   heightCm);
    prefs.end();

    Serial.printf("[Calories] Profile saved — age=%d weight=%.1f height=%.1f\n",
                  age, weightKg, heightCm);
}

bool  Calories::isProfileSet() { return profileSet; }
int   Calories::getAge()       { return age; }
float Calories::getWeightKg()  { return weightKg; }
float Calories::getHeightCm()  { return heightCm; }
float Calories::getKcal()      { return totalKcal; }

void Calories::reset() {
    totalKcal = 0.0f;
    Serial.println("[Calories] Reset.");
}

// age-based fitness multiplier — older = slightly higher calorie burn
// for the same effort due to reduced metabolic efficiency.
// range is roughly 0.95 (young) to 1.10 (older adult).
float Calories::ageFitnessMultiplier() {
    if (age < 20)       return 0.95f;
    else if (age < 30)  return 1.00f;
    else if (age < 40)  return 1.03f;
    else if (age < 50)  return 1.06f;
    else if (age < 60)  return 1.08f;
    else                return 1.10f;
}

// MET value from walking/running speed in m/s.
// based on ACSM metabolic equations and published MET compendium values.
float Calories::metFromSpeed(float speedMps) {
    if (speedMps < 0.5f)  return 1.0f;   // stationary / very slow
    if (speedMps < 1.2f)  return 2.0f;   // slow walk
    if (speedMps < 1.8f)  return 3.5f;   // normal walk
    if (speedMps < 2.2f)  return 4.5f;   // brisk walk
    if (speedMps < 2.8f)  return 6.0f;   // fast walk / light jog
    if (speedMps < 3.5f)  return 8.0f;   // jog
    return 10.0f;                         // running
}

void Calories::onStep(const char* pace) {
    // estimate cadence (steps/min) from pace label — gives a more realistic
    // speed than assuming a flat 100 steps/min for every activity level
    if (strstr(pace, "RUNNING") || strstr(pace, "Run")) {
        stepsPerMin = 155.0f;
    } else if (strstr(pace, "WALKING") || strstr(pace, "Walk")) {
        stepsPerMin = 100.0f;
    } else {
        stepsPerMin = 60.0f;   // slow / standing
    }

    // stride length scales with height — standard anthropometric ratio
    float strideLenM = heightCm * 0.00414f;   // metres per step

    // speed in m/s from stride length and cadence
    float speedMps = strideLenM * (stepsPerMin / 60.0f);

    // MET from speed bands, scaled by age multiplier
    float met = metFromSpeed(speedMps) * ageFitnessMultiplier();

    // time this step represents in hours
    float hrsPerStep = 1.0f / stepsPerMin / 60.0f;

    // standard kcal formula: MET * weight(kg) * time(hrs)
    totalKcal += met * weightKg * hrsPerStep;
}
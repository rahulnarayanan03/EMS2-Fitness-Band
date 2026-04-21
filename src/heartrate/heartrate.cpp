#include <Arduino.h>
#include "HeartRate.h"
#include <heartRate.h>

static const byte RATE_SIZE = 4;
static byte rates[RATE_SIZE];
static byte rateSpot = 0;
static long lastBeat = 0;
static float beatsPerMinute = 0;
static int beatAvg = 0;

void HeartRate::update(long irValue) {

    if (irValue < minIR) {
        beatsPerMinute = 0;
        beatAvg = 0;
        lastBeat = 0;
        return;
    }

    if (checkForBeat(irValue) == true) {
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            rates[rateSpot++] = (byte)beatsPerMinute;
            rateSpot %= RATE_SIZE;

            beatAvg = 0;
            for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
            beatAvg /= RATE_SIZE;
        }
    }

    bpm = beatAvg;
}

int HeartRate::getBPM() {
    return bpm;
}
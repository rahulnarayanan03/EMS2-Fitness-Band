/*Shows stand.png, walk.gif or run.gif depending on the user's step rate.
 Images are compiled into flash as C arrays (see anim_data.h) so no
 filesystem setup is needed.

 Activity thresholds (steps per minute):
   standing  ->   0 - 29  spm
   walking   ->  30 - 99  spm
   running   ->  100+     spm
*/

#ifndef ANIMATION_H
#define ANIMATION_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>
#include <PNGdec.h>
#include "anim_data.h"

// activity thresholds in steps per minute - adjust after physical testing
static constexpr uint16_t ANIM_WALK_THRESHOLD = 30;
static constexpr uint16_t ANIM_RUN_THRESHOLD  = 100;

// how long each GIF frame is shown before moving to the next (ms)
static constexpr uint32_t ANIM_WALK_FRAME_MS = 120;
static constexpr uint32_t ANIM_RUN_FRAME_MS  = 80;

// step rate is recalculated over this window
static constexpr uint32_t RATE_WINDOW_MS = 5000;

enum class ActivityState { STANDING, WALKING, RUNNING };

class Animation {
public:
    // pass the shared tft and the top-left pixel where the sprite should sit
    Animation(TFT_eSPI &tft, int16_t x, int16_t y);

    // call once after tft.init() and after any full fillScreen so the
    // first frame lands on a clean background
    void begin();

    // call every loop() - handles rate calculation, state switching and
    // advancing GIF frames automatically
    void update(uint32_t stepCount, uint32_t nowMs);

    ActivityState getState() const { return _state; }

private:
    TFT_eSPI   &_tft;
    int16_t     _x, _y;

    AnimatedGIF _gif;
    PNG         _png;

    // step rate tracking
    uint32_t    _prevStepCount   = 0;
    uint32_t    _prevRateCheckMs = 0;
    uint16_t    _stepsPerMinute  = 0;

    // state
    ActivityState _state         = ActivityState::STANDING;
    ActivityState _lastState     = (ActivityState)255; // force first draw

    // GIF frame tracking
    int         _gifFrame        = 0;
    uint32_t    _lastFrameMs     = 0;
    int         _totalGifFrames  = 0;

    void updateState();
    void drawCurrentState(uint32_t nowMs);
    void drawStandingPng();
    void drawGifFrame(const uint8_t *gifData, size_t gifLen,
                      uint32_t frameIntervalMs, uint32_t nowMs);

    // static callbacks required by AnimatedGIF and PNGdec
    static void gifDraw(GIFDRAW *pDraw);
    static int pngDraw(PNGDRAW *pDraw);

    // pointer back to self so static callbacks can reach instance members
    static Animation *_instance;
};

#endif // ANIMATION_H
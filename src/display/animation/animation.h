/* Displays a stick figure animation that reflects the user's current activity level.
Determines standing / walking / running from the step rate, then cycles through
the matching animation frames so the figure actually moves on screen.

Activity thresholds (steps per minute):
  standing  ->  0 - 29  spm
  walking   ->  30 - 99 spm
  running   ->  100+    spm
*/

#ifndef ANIMATION_H
#define ANIMATION_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// --- activity thresholds (steps per minute) ---
static constexpr uint16_t ANIM_WALK_THRESHOLD = 30;   // below this = standing
static constexpr uint16_t ANIM_RUN_THRESHOLD  = 100;  // above this = running

// how often to advance the animation frame (ms)
static constexpr uint32_t ANIM_FRAME_INTERVAL_STAND = 0;    // standing doesn't cycle
static constexpr uint32_t ANIM_FRAME_INTERVAL_WALK  = 400;  // walk cadence
static constexpr uint32_t ANIM_FRAME_INTERVAL_RUN   = 180;  // run cadence

enum class ActivityState {
    STANDING,
    WALKING,
    RUNNING
};

class Animation {
public:
    // pass in the shared TFT instance; x/y are the top-left corner of the
    // drawing area, which should not overlap with the step counter region
    Animation(TFT_eSPI &tft, int16_t x, int16_t y);

    // call once in setup() - draws the initial frame
    void begin();

    // call every loop() - pass the latest step count and current millis()
    // the class tracks rate internally so you don't have to
    void update(uint32_t stepCount, uint32_t nowMs);

    // returns whichever state is currently active
    ActivityState getState() const;

private:
    TFT_eSPI  &_tft;
    int16_t    _x;
    int16_t    _y;

    // step rate tracking
    uint32_t   _prevStepCount    = 0;
    uint32_t   _prevRateCheckMs  = 0;
    uint16_t   _stepsPerMinute   = 0;
    static constexpr uint32_t RATE_WINDOW_MS = 5000; // check rate every 5 s

    // animation state
    ActivityState _state         = ActivityState::STANDING;
    ActivityState _lastDrawnState = (ActivityState)255; // forces first draw
    uint8_t    _frame            = 0;
    uint32_t   _lastFrameMs      = 0;

    // how many frames each activity has
    static constexpr uint8_t FRAMES_STAND = 1;
    static constexpr uint8_t FRAMES_WALK  = 4;
    static constexpr uint8_t FRAMES_RUN   = 4;

    // internal helpers
    void updateState();
    void advanceFrame(uint32_t nowMs);
    void drawFrame();
    void eraseArea();

    // one draw function per activity, parameterised by frame index
    void drawStanding();
    void drawWalking(uint8_t frame);
    void drawRunning(uint8_t frame);

    // low-level drawing helpers - all coords relative to _x/_y
    void drawHead(int16_t cx, int16_t cy, int16_t r, uint16_t colour);
    void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t colour);
};

#endif // ANIMATION_H
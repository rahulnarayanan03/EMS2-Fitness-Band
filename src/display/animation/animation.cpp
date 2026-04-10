#include "animation.h"

// the stick figure fits in a ~60x80px box
// these are offsets from the top-left corner passed in the constructor
static constexpr int16_t HEAD_R    = 8;    // head circle radius
static constexpr int16_t HEAD_CX   = 30;   // head centre x (relative to _x)
static constexpr int16_t HEAD_CY   = 12;   // head centre y (relative to _y)
static constexpr int16_t NECK_Y    = 20;   // where the body line starts
static constexpr int16_t HIP_Y     = 50;   // where legs branch off
static constexpr int16_t BODY_CX   = 30;   // x of the torso centreline
static constexpr int16_t AREA_W    = 60;   // erase rect width
static constexpr int16_t AREA_H    = 85;   // erase rect height

Animation::Animation(TFT_eSPI &tft, int16_t x, int16_t y)
    : _tft(tft), _x(x), _y(y) {}

void Animation::begin() {
    eraseArea();
    drawFrame();
}

void Animation::update(uint32_t stepCount, uint32_t nowMs) {
    // recalculate step rate every RATE_WINDOW_MS milliseconds
    if (nowMs - _prevRateCheckMs >= RATE_WINDOW_MS) {
        uint32_t elapsed = nowMs - _prevRateCheckMs;
        uint32_t delta   = stepCount - _prevStepCount;

        // convert delta steps over the elapsed window to steps per minute
        _stepsPerMinute = (uint16_t)((delta * 60000UL) / elapsed);

        _prevStepCount   = stepCount;
        _prevRateCheckMs = nowMs;

        updateState();
    }

    advanceFrame(nowMs);

    // only redraw if the state changed or the frame index advanced
    // (advanceFrame sets _frame and bumps _lastFrameMs when it ticks)
    drawFrame();
}

ActivityState Animation::getState() const {
    return _state;
}

// ---- private methods -------------------------------------------------------

void Animation::updateState() {
    if (_stepsPerMinute >= ANIM_RUN_THRESHOLD) {
        _state = ActivityState::RUNNING;
    } else if (_stepsPerMinute >= ANIM_WALK_THRESHOLD) {
        _state = ActivityState::WALKING;
    } else {
        _state = ActivityState::STANDING;
    }

    // reset to frame 0 whenever we switch activity so it doesn't start mid-cycle
    if (_state != _lastDrawnState) {
        _frame = 0;
    }
}

void Animation::advanceFrame(uint32_t nowMs) {
    uint32_t interval = 0;
    uint8_t  maxFrames = 1;

    switch (_state) {
        case ActivityState::WALKING:
            interval  = ANIM_FRAME_INTERVAL_WALK;
            maxFrames = FRAMES_WALK;
            break;
        case ActivityState::RUNNING:
            interval  = ANIM_FRAME_INTERVAL_RUN;
            maxFrames = FRAMES_RUN;
            break;
        default: // standing - no cycling needed
            return;
    }

    if (nowMs - _lastFrameMs >= interval) {
        _lastFrameMs = nowMs;
        _frame = (_frame + 1) % maxFrames;
    }
}

void Animation::drawFrame() {
    // skip the draw if nothing has actually changed
    bool stateChanged = (_state != _lastDrawnState);
    // for animated states we always redraw each frame tick (handled by advanceFrame)
    // for standing we only redraw on state change
    if (!stateChanged && _state == ActivityState::STANDING) return;

    eraseArea();

    switch (_state) {
        case ActivityState::STANDING: drawStanding();        break;
        case ActivityState::WALKING:  drawWalking(_frame);   break;
        case ActivityState::RUNNING:  drawRunning(_frame);   break;
    }

    _lastDrawnState = _state;
}

void Animation::eraseArea() {
    _tft.fillRect(_x, _y, AREA_W, AREA_H, TFT_BLACK);
}

// ---- standing pose ---------------------------------------------------------
// upright torso, arms slightly out, legs straight down

void Animation::drawStanding() {
    uint16_t col = TFT_CYAN;

    drawHead(HEAD_CX, HEAD_CY, HEAD_R, col);

    // torso
    drawLine(BODY_CX, NECK_Y, BODY_CX, HIP_Y, col);

    // arms hanging down at a slight angle
    drawLine(BODY_CX, 28, BODY_CX - 12, 42, col); // left arm
    drawLine(BODY_CX, 28, BODY_CX + 12, 42, col); // right arm

    // legs straight down
    drawLine(BODY_CX, HIP_Y, BODY_CX - 10, 75, col); // left leg
    drawLine(BODY_CX, HIP_Y, BODY_CX + 10, 75, col); // right leg
}

// ---- walking pose ----------------------------------------------------------
// 4 frames that cycle the arms and legs forward/back like a walking stride

void Animation::drawWalking(uint8_t frame) {
    uint16_t col = TFT_GREEN;

    // arm and leg angles shift across the 4 frames
    // positive = swung forward (towards +x), negative = swung back
    // left limbs and right limbs are always opposite phase

    // arm offsets at hip level (applied to x)
    const int8_t armSwingX[4] = { -10, -5, 10, 5 };
    const int8_t armSwingY[4] = {  42, 38, 42, 38 };

    // leg foot positions
    const int8_t legFwdX[4]   = {  14,  8, -14, -8 };
    const int8_t legBckX[4]   = { -14, -8,  14,  8 };

    drawHead(HEAD_CX, HEAD_CY, HEAD_R, col);
    drawLine(BODY_CX, NECK_Y, BODY_CX, HIP_Y, col); // torso

    // arms - left swings opposite to right
    drawLine(BODY_CX, 28, BODY_CX + armSwingX[frame],     armSwingY[frame], col);
    drawLine(BODY_CX, 28, BODY_CX - armSwingX[frame],     armSwingY[frame], col);

    // legs
    drawLine(BODY_CX, HIP_Y, BODY_CX + legFwdX[frame], 75, col);
    drawLine(BODY_CX, HIP_Y, BODY_CX + legBckX[frame], 75, col);
}

// ---- running pose ----------------------------------------------------------
// 4 frames with exaggerated arm drive and high knee lift

void Animation::drawRunning(uint8_t frame) {
    uint16_t col = TFT_ORANGE;

    // leaning torso - body tilts slightly forward when running
    // neck x shifts by a couple of pixels
    const int8_t neckOffX[4] = { 2, 3, 2, 1 };

    // arm drive - much wider swing than walking
    const int8_t armEndX[4] = { -18, -6, 18,  6 };
    const int8_t armEndY[4] = {  36, 44, 36, 44 };

    // legs - one knee high (short line up), the other extending back (long line down/back)
    // forward leg: knee comes up, foot is tucked
    const int8_t legFwdKneeX[4] = {  10,  4, -10, -4 };
    const int8_t legFwdKneeY[4] = {  58, 62,  58, 62 };
    const int8_t legFwdFootX[4] = {  16,  6, -10, -4 };
    const int8_t legFwdFootY[4] = {  75, 70,  68, 72 };

    // back leg: straight and pushing off
    const int8_t legBckFootX[4] = { -14, -8,  14,  8 };

    int16_t neckX = BODY_CX + neckOffX[frame];

    drawHead(neckX, HEAD_CY, HEAD_R, col);
    drawLine(neckX, NECK_Y, BODY_CX, HIP_Y, col); // slightly leaning torso

    // arms
    drawLine(BODY_CX, 30, BODY_CX + armEndX[frame], armEndY[frame], col);
    drawLine(BODY_CX, 30, BODY_CX - armEndX[frame], armEndY[frame], col);

    // forward (bent) leg - two segments: hip->knee, knee->foot
    drawLine(BODY_CX, HIP_Y,
             BODY_CX + legFwdKneeX[frame], legFwdKneeY[frame], col);
    drawLine(BODY_CX + legFwdKneeX[frame], legFwdKneeY[frame],
             BODY_CX + legFwdFootX[frame], legFwdFootY[frame], col);

    // back leg - straight push-off
    drawLine(BODY_CX, HIP_Y, BODY_CX + legBckFootX[frame], 75, col);
}

// ---- low-level helpers -----------------------------------------------------

void Animation::drawHead(int16_t cx, int16_t cy, int16_t r, uint16_t colour) {
    _tft.drawCircle(_x + cx, _y + cy, r, colour);
}

void Animation::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t colour) {
    _tft.drawLine(_x + x1, _y + y1, _x + x2, _y + y2, colour);
}
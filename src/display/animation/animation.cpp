/* See animation.h for the full description and threshold values.
   Images live in anim_data.h as PROGMEM byte arrays - no SD or SPIFFS needed.
*/

#include "animation.h"

// sprite dimensions - must match the actual files
static constexpr int16_t SPRITE_W = 65;
static constexpr int16_t SPRITE_H = 75;

// static instance pointer so the library callbacks can reach us
Animation *Animation::_instance = nullptr;

// ---- constructor ------------------------------------------------------------

Animation::Animation(TFT_eSPI &tft, int16_t x, int16_t y)
    : _tft(tft), _x(x), _y(y) {
    _instance = this;
}

// ---- public -----------------------------------------------------------------

void Animation::begin() {
    _gif.begin(GIF_PALETTE_RGB565_BE); // RGB565 big-endian matches TFT_eSPI
    drawStandingPng();
}

void Animation::update(uint32_t stepCount, uint32_t nowMs) {
    // recalculate step rate every RATE_WINDOW_MS
    if (nowMs - _prevRateCheckMs >= RATE_WINDOW_MS) {
        uint32_t elapsed    = nowMs - _prevRateCheckMs;
        uint32_t delta      = stepCount - _prevStepCount;
        _stepsPerMinute     = (uint16_t)((delta * 60000UL) / elapsed);
        _prevStepCount      = stepCount;
        _prevRateCheckMs    = nowMs;
        updateState();
    }

    drawCurrentState(nowMs);
}

// ---- private ----------------------------------------------------------------

void Animation::updateState() {
    ActivityState next;

    if (_stepsPerMinute >= ANIM_RUN_THRESHOLD) {
        next = ActivityState::RUNNING;
    } else if (_stepsPerMinute >= ANIM_WALK_THRESHOLD) {
        next = ActivityState::WALKING;
    } else {
        next = ActivityState::STANDING;
    }

    // on state change reset GIF back to frame 0 and erase the sprite area
    if (next != _state) {
        _state    = next;
        _gifFrame = 0;
        _lastFrameMs = 0;
        _tft.fillRect(_x, _y, SPRITE_W, SPRITE_H, TFT_BLACK);
    }
}

void Animation::drawCurrentState(uint32_t nowMs) {
    switch (_state) {
        case ActivityState::STANDING:
            // only redraw the PNG when the state first switches to standing
            if (_lastState != ActivityState::STANDING) {
                drawStandingPng();
            }
            break;

        case ActivityState::WALKING:
            drawGifFrame(walk_gif, walk_gif_len, ANIM_WALK_FRAME_MS, nowMs);
            break;

        case ActivityState::RUNNING:
            drawGifFrame(run_gif, run_gif_len, ANIM_RUN_FRAME_MS, nowMs);
            break;
    }

    _lastState = _state;
}

// draws the static stand.png once
void Animation::drawStandingPng() {
    // open the PNG from the PROGMEM array
    int rc = _png.openFLASH((uint8_t *)stand_png, stand_png_len, pngDraw);
    if (rc == PNG_SUCCESS) {
        _png.decode(nullptr, 0);
        _png.close();
    }
}

// advances and draws one GIF frame when the frame interval has elapsed
void Animation::drawGifFrame(const uint8_t *gifData, size_t gifLen,
                              uint32_t frameIntervalMs, uint32_t nowMs) {
    if (nowMs - _lastFrameMs < frameIntervalMs) return;
    _lastFrameMs = nowMs;

    // open returns the frame count; reopen each time to seek to the right frame
    // AnimatedGIF on PROGMEM is lightweight enough that this is fine
    int frameCount = _gif.open((uint8_t *)gifData, gifLen, gifDraw);
    if (frameCount <= 0) return;

    // seek to the current frame
    for (int i = 0; i < _gifFrame; i++) {
        if (!_gif.playFrame(false, nullptr)) break;
    }

    // draw it
    _gif.playFrame(false, nullptr);
    _gif.close();

    _gifFrame = (_gifFrame + 1) % frameCount;
}

// ---- AnimatedGIF callback ---------------------------------------------------
// called per scanline row by the library; writes pixels directly to the TFT

void Animation::gifDraw(GIFDRAW *pDraw) {
    if (!_instance) return;

    Animation &self = *_instance;
    int16_t y = self._y + pDraw->iY + pDraw->y;

    // pDraw->pPixels is already RGB565 because we opened with GIF_PALETTE_RGB565_BE
    uint16_t *pixels = (uint16_t *)pDraw->pPixels;

    self._tft.pushImage(self._x + pDraw->iX, y, pDraw->iWidth, 1, pixels);
}

// ---- PNGdec callback --------------------------------------------------------
// called per scanline row; converts the decoded row to RGB565 and pushes it

int Animation::pngDraw(PNGDRAW *pDraw) {
    if (!_instance) return 0;

    Animation &self = *_instance;

    // decode the row into a local RGB565 line buffer
    uint16_t lineBuf[SPRITE_W];
    self._png.getLineAsRGB565(pDraw, lineBuf, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    self._tft.pushImage(self._x, self._y + pDraw->y, pDraw->iWidth, 1, lineBuf);
    return 1;
}
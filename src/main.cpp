#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

#define TOUCH_CS  33
#define TOUCH_IRQ 36

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700;
uint16_t touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

void setup() {
    Serial.begin(9600);
    delay(2000);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("EMS2 Testing");

    touchSPI.begin(25, 39, 32);
    SPI.begin(25, 39, 32);
    ts.begin();
    ts.setRotation(1);

    Serial.println("Touch test ready.");
}

void loop() {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();

        if (p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
        if (p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
        if (p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
        if (p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;

        uint16_t x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 0, tft.width());
        uint16_t y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 0, tft.height());

        tft.fillCircle(x, y, 5, TFT_RED);

        Serial.print("Touch at: ");
        Serial.print(x);
        Serial.print(", ");
        Serial.println(y);

        delay(100);
    }
}
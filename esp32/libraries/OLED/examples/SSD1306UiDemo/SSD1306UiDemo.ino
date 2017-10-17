#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"

#include "OLEDDisplayUi.h"
#include "images.h"

SSD1306  display(0x3c, 4, 15);

#define DEMO_DURATION 3000
typedef void (*Demo)(void);

OLEDDisplayUi ui     ( &display );

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis()));
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x, y, BT_width, BT_height, BT_bits);
  display->drawXbm(x + 12 + 1, y, WIFI_width, WIFI_height, WIFI_bits);
  display->drawXbm(x + 108, y, BAT_width, BAT_height, BAT_bits);
  display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x, y, BT_width, BT_height, BT_bits);
  display->drawXbm(x + 12 + 1, y, WIFI_width, WIFI_height, WIFI_bits);
  display->drawXbm(x + 108, y, BAT_width, BAT_height, BAT_bits);
  display->drawXbm(x + 34, y + 12, LoRa_Logo_width, LoRa_Logo_height, LoRa_Logo_bits);
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x, y + 5, HelTec_LOGO_width, HelTec_LOGO_height, HelTec_LOGO_bits);
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y, "HelTec");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x, y + 25, "HelTec AutoMation");
  display->drawString(x, y + 35, "www.heltec.cn");
}

FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4 };

int frameCount = 4;

//OverlayCallback overlays[] = { msOverlay };
//int overlaysCount = 1;

void setup() {
  Serial.begin(115200);
  
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);   // turn the LED on (HIGH is the voltage level)
  delay(100);                       // wait for a second
  digitalWrite(16, HIGH);    // turn the LED off by making the voltage LOW
  
  ui.setTargetFPS(30);

  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
//  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();
}

void loop() {
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }
 }

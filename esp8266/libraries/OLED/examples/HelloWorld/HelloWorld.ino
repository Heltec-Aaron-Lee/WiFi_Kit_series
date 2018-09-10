// This example just provide basic function test;
// For more informations, please vist www.heltec.cn or mail to support@heltec.cn

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`

//OLED pins to ESP32 GPIOs via this connecthin:
//OLED_SDA -- GPIO4
//OLED_SCL -- GPIO5
//OLED_RST -- GPIO16

SSD1306  display(0x3c, SDA, SCL, OLED_RST, GEOMETRY_128_32);

void setup() {
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  display.drawString(0,0,"Hello World!");
  display.display();
}

void loop() { }

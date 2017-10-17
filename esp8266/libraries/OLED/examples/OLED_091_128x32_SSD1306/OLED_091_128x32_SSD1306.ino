// This example just provide basic function test;
// For more informations, please vist www.heltec.cn or mail to support@heltec.cn

#include <Wire.h>
#include "OLED.h"

//WIFI_Kit_8's OLED connection:
//SDA -- GPIO4 -- D2
//SCL -- GPIO5 -- D1
//RST -- GPIO16 -- D0

#define RST_OLED 16
OLED display(4, 5);

// If you bought WIFI Kit 8 before 2017-8-20, you may try this initial
//#define RST_OLED D2
//OLED display(SDA, SCL);

void setup() {
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, LOW);   // turn D2 low to reset OLED
  delay(50);
  digitalWrite(RST_OLED, HIGH);    // while OLED is running, must set D2 in high
  
  Serial.begin(9600);
  Serial.println("OLED test!");

  // Initialize display
  display.begin();

  // Test message
  display.print("Hello World");
  delay(3*1000);

  // Test long message
  display.print("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
  delay(3*1000);

  // Test display clear
  display.clear();
  delay(3*1000);

  // Test message postioning
  display.print("TOP-LEFT");
  display.print("4th row", 4);
  display.print("RIGHT-BOTTOM", 7, 4);
  delay(3*1000);

  // Test display OFF
  display.off();
  display.print("3rd row", 3, 8);
  delay(3*1000);

  // Test display ON
  display.on();
  delay(3*1000);
}

int r = 0, c = 0;

void loop() {
  
    r = r % 4;
    c = micros() % 6;
    if (r == 0) display.clear();
    display.print("Hello World", r++, c++);
    delay(500);
}

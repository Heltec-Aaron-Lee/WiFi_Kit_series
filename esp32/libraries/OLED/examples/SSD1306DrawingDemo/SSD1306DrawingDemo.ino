// This example just provide basic function test;
// For more informations, please vist www.heltec.cn or mail to support@heltec.cn

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`

//OLED pins to ESP32 GPIOs via this connecthin:
//OLED_SDA -- GPIO4
//OLED_SCL -- GPIO15
//OLED_RST -- GPIO16

#define SDA		4
#define	SCL		15
#define	RST		16 //RST must be set by software

#define V2 	1

#ifdef V2 //WIFI Kit series V1 not support Vext control
	#define Vext	21
#endif

#define DISPLAY_HEIGHT 64
#define DISPLAY_WIDTH  128

SSD1306  display(0x3c, SDA, SCL, RST);

// Adapted from Adafruit_SSD1306
void drawLines() {
  for (int16_t i=0; i<DISPLAY_WIDTH; i+=4) {
    display.drawLine(0, 0, i, DISPLAY_HEIGHT-1);
    display.display();
    delay(10);
  }
  for (int16_t i=0; i<DISPLAY_HEIGHT; i+=4) {
    display.drawLine(0, 0, DISPLAY_WIDTH-1, i);
    display.display();
    delay(10);
  }
  delay(250);

  display.clear();
  for (int16_t i=0; i<DISPLAY_WIDTH; i+=4) {
    display.drawLine(0, DISPLAY_HEIGHT-1, i, 0);
    display.display();
    delay(10);
  }
  for (int16_t i=DISPLAY_HEIGHT-1; i>=0; i-=4) {
    display.drawLine(0, DISPLAY_HEIGHT-1, DISPLAY_WIDTH-1, i);
    display.display();
    delay(10);
  }
  delay(250);

  display.clear();
  for (int16_t i=DISPLAY_WIDTH-1; i>=0; i-=4) {
    display.drawLine(DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1, i, 0);
    display.display();
    delay(10);
  }
  for (int16_t i=DISPLAY_HEIGHT-1; i>=0; i-=4) {
    display.drawLine(DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1, 0, i);
    display.display();
    delay(10);
  }
  delay(250);
  display.clear();
  for (int16_t i=0; i<DISPLAY_HEIGHT; i+=4) {
    display.drawLine(DISPLAY_WIDTH-1, 0, 0, i);
    display.display();
    delay(10);
  }
  for (int16_t i=0; i<DISPLAY_WIDTH; i+=4) {
    display.drawLine(DISPLAY_WIDTH-1, 0, i, DISPLAY_HEIGHT-1);
    display.display();
    delay(10);
  }
  delay(250);
}

// Adapted from Adafruit_SSD1306
void drawRect(void) {
  for (int16_t i=0; i<DISPLAY_HEIGHT/2; i+=2) {
    display.drawRect(i, i, DISPLAY_WIDTH-2*i, DISPLAY_HEIGHT-2*i);
    display.display();
    delay(10);
  }
}

// Adapted from Adafruit_SSD1306
void fillRect(void) {
  uint8_t color = 1;
  for (int16_t i=0; i<DISPLAY_HEIGHT/2; i+=3) {
    display.setColor((color % 2 == 0) ? BLACK : WHITE); // alternate colors
    display.fillRect(i, i, DISPLAY_WIDTH - i*2, DISPLAY_HEIGHT - i*2);
    display.display();
    delay(10);
    color++;
  }
  // Reset back to WHITE
  display.setColor(WHITE);
}

// Adapted from Adafruit_SSD1306
void drawCircle(void) {
  for (int16_t i=0; i<DISPLAY_HEIGHT; i+=2) {
    display.drawCircle(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, i);
    display.display();
    delay(10);
  }
  delay(1000);
  display.clear();

  // This will draw the part of the circel in quadrant 1
  // Quadrants are numberd like this:
  //   0010 | 0001
  //  ------|-----
  //   0100 | 1000
  //
  display.drawCircleQuads(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/4, 0b00000001);
  display.display();
  delay(200);
  display.drawCircleQuads(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/4, 0b00000011);
  display.display();
  delay(200);
  display.drawCircleQuads(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/4, 0b00000111);
  display.display();
  delay(200);
  display.drawCircleQuads(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/4, 0b00001111);
  display.display();
}

void printBuffer(void) {
  // Initialize the log buffer
  // allocate memory to store 8 lines of text and 30 chars per line.
  display.setLogBuffer(5, 30);

  // Some test data
  const char* test[] = {
      "Hello",
      "World" ,
      "----",
      "Show off",
      "how",
      "the log buffer",
      "is",
      "working.",
      "Even",
      "scrolling is",
      "working"
  };

  for (uint8_t i = 0; i < 11; i++) {
    display.clear();
    // Print to the screen
    display.println(test[i]);
    // Draw it to the internal screen buffer
    display.drawLogBuffer(0, 0);
    // Display it on the screen
    display.display();
    delay(500);
  }
}

void setup() {
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);    // OLED USE Vext as power supply, must turn ON Vext before OLED init
  delay(50); 
  
  display.init();

  // display.flipScreenVertically();

  display.setContrast(255);

  drawLines();
  delay(1000);
  display.clear();

  drawRect();
  delay(1000);
  display.clear();

  fillRect();
  delay(1000);
  display.clear();

  drawCircle();
  delay(1000);
  display.clear();

  printBuffer();
  delay(1000);
  display.clear();
}

void loop() { }

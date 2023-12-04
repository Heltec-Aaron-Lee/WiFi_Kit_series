#include "HT_lCMEN2R13EFC1.h"
#include "images.h"

// Initialize the display
HT_ICMEN2R13EFC1   display(6, 5, 4,7, 3,2,-1, 6000000);//rst,dc,cs,busy,sck,mosi,miso,frequency

typedef void (*Demo)(void);

/* screen rotation
 * ANGLE_0_DEGREE
 * ANGLE_90_DEGREE
 * ANGLE_180_DEGREE
 * ANGLE_270_DEGREE
 */
#define DIRECTION ANGLE_0_DEGREE

int width,height;
int demoMode = 0;

void setup() {
  Serial.begin(115200);
  if (DIRECTION==ANGLE_0_DEGREE||DIRECTION==ANGLE_180_DEGREE)
  {
    width = display._width;
    height = display._height;
  }
  else
  {
    width = display._height;
    height = display._width;
  }
  VextON();
  delay(100);

  // Initialising the UI will init the display too.
  display.init();
  display.screenRotate(DIRECTION);
  display.setFont(ArialMT_Plain_10);
}

void drawFontFaceDemo() {
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Hello world");

    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 26, "Hello world");
//    display.update(BLACK_BUFFER);

    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "Hello world");

    display.update(BLACK_BUFFER);
    display.display();
}

void drawTextFlowDemo() {

    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(0, 0, 128,
      "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore." );
      
    display.update(BLACK_BUFFER);
    
    display.update(COLOR_BUFFER);

    display.display();
}

void drawTextAlignmentDemo() {
  // Text alignment demo

  display.clear();
  char str[30];
  int x = 0;
  int y = 0;
  display.setFont(ArialMT_Plain_10);
  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(x, y, "Left aligned (0,0)");

  // The coordinates define the right end of the text
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  x = width;
  y = height-12;
  sprintf(str,"Right aligned (%d,%d)",x,y);
  display.drawString(x, y, str);

  display.update(BLACK_BUFFER);
  
  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  x = width/2;
  y = height/2-5;
  sprintf(str,"Center aligned (%d,%d)",x,y);
  display.drawString(x, y, str);

  display.update(COLOR_BUFFER);
  
  display.display();    
}

void drawRectDemo() {

    display.clear();
      // Draw a pixel at given position
    for (int i = 0; i < 10; i++) {
      display.setPixel(i, i);
      display.setPixel(10 - i, i);
    }
    display.drawRect(12, 12, 20, 20);

    // Fill the rectangle
    display.fillRect(14, 14, 17, 17);

    // Draw a line horizontally
    display.drawHorizontalLine(0, 40, 20);

    // Draw a line horizontally
    display.drawVerticalLine(40, 0, 20);
    display.update(BLACK_BUFFER);

    
    display.update(COLOR_BUFFER);
    display.display();
}

void drawCircleDemo() {
  int x = width/4;
  int y = height/2;
  display.clear();
  for (int i=1; i < 8; i++) {
    display.setColor(WHITE);
    display.drawCircle(x, y, i*3);
    if (i % 2 == 0) {
      display.setColor(BLACK);
    }
  }
  display.update(BLACK_BUFFER);
  
//  display.clear();
  x = width/4*3;
  for (int i=1; i < 8; i++) {
    display.setColor(WHITE);
    if (i % 2 == 0) {
      display.setColor(BLACK);
    }
    display.fillCircle(x, y, 32 - i* 3);
  }
  display.update(COLOR_BUFFER);
  display.display();
}


void drawImageDemo() {
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
    display.clear();
    display.update(BLACK_BUFFER);

    display.clear();
    int x = width/2-WiFi_Logo_width/2;
    int y = height/2-WiFi_Logo_height/2;
    display.drawXbm(x ,y  , WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
    display.update(COLOR_BUFFER);
    display.display();
}

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}


Demo demos[] = {drawFontFaceDemo, drawTextFlowDemo, drawTextAlignmentDemo, drawRectDemo, drawCircleDemo, drawImageDemo};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;

void loop() {
  // draw the current demo method
  demos[demoMode]();

  demoMode = (demoMode + 1)  % demoLength;
  delay(15000);
}

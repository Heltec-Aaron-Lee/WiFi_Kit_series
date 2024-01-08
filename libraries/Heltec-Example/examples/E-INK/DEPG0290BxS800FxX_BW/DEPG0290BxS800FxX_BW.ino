#include "HT_DEPG0290BxS800FxX_BW.h"
#include "images.h"

// Initialize the display
DEPG0290BxS800FxX_BW   display(GPIO1, GPIO2, GPIO3,GPIO5, SCK,MOSI,MISO, 6000000);//rst,dc,cs,busy,sck,mosi,miso,frequency

typedef void (*Demo)(void);

/* screen rotation
 * ANGLE_0_DEGREE
 * ANGLE_90_DEGREE
 * ANGLE_180_DEGREE
 * ANGLE_270_DEGREE
 */
#define DIRECTION ANGLE_0_DEGREE
int demoMode = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
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
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Hello world");
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "Hello world");
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 26, "Hello world");
}

void drawTextFlowDemo() {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(0, 0, 128,
      "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore." );
}

void drawTextAlignmentDemo() {
  // Text alignment demo
  char str[30];
  int x = 0;
  int y = 0;
  display.setFont(ArialMT_Plain_10);
  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(x, y, "Left aligned (0,0)");

  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  x = display.width()/2;
  y = display.height()/2-5;
  sprintf(str,"Center aligned (%d,%d)",x,y);
  display.drawString(x, y, str);

  // The coordinates define the right end of the text
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  x = display.width();
  y = display.height()-12;
  sprintf(str,"Right aligned (%d,%d)",x,y);
  display.drawString(x, y, str);
}

void drawRectDemo() {
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
}

void drawCircleDemo() {
  int x = display.width()/4;
  int y = display.height()/2;
  for (int i=1; i < 8; i++) {
    display.setColor(WHITE);
    display.drawCircle(x, y, i*3);
    if (i % 2 == 0) {
      display.setColor(BLACK);
    }
    int x = display.width()/4*3;
    display.fillCircle(x, y, 32 - i* 3);
  }
}


void drawImageDemo() {
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
    int x = display.width()/2-WiFi_Logo_width/2;
    int y = display.height()/2-WiFi_Logo_height/2;
    display.drawXbm(x ,y  , WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
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
  // clear the display
  display.clear();
  // draw the current demo method
  demos[demoMode]();
  display.display();

  demoMode = (demoMode + 1)  % demoLength;
  delay(5000);
}
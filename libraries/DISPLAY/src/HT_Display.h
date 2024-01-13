#ifndef DISPLAY_h
#define DISPLAY_h


#include <Arduino.h>
#include <HT_DisplayFonts.h>

//#define DEBUG_DISPLAY(...) Serial.printf( __VA_ARGS__ )
//#define DEBUG_DISPLAY(...) dprintf("%s",  __VA_ARGS__ )

#ifndef DEBUG_DISPLAY
#define DEBUG_DISPLAY(...)
#endif

// Use DOUBLE BUFFERING by default
#ifndef DISPLAY_REDUCE_MEMORY
#define DISPLAY_DOUBLE_BUFFER
#endif

// Header Values
#define JUMPTABLE_BYTES 4

#define JUMPTABLE_LSB   1
#define JUMPTABLE_SIZE  2
#define JUMPTABLE_WIDTH 3
#define JUMPTABLE_START 4

#define WIDTH_POS 0
#define HEIGHT_POS 1
#define FIRST_CHAR_POS 2
#define CHAR_NUM_POS 3


// Display commands
#define CHARGEPUMP 0x8D
#define COLUMNADDR 0x21
#define COMSCANDEC 0xC8
#define COMSCANINC 0xC0
#define DISPLAYALLON 0xA5
#define DISPLAYALLON_RESUME 0xA4
#define DISPLAYOFF 0xAE
#define DISPLAYON 0xAF
#define EXTERNALVCC 0x1
#define INVERTDISPLAY 0xA7
#define MEMORYMODE 0x20
#define NORMALDISPLAY 0xA6
#define PAGEADDR 0x22
#define SEGREMAP 0xA0
#define SETCOMPINS 0xDA
#define SETCONTRAST 0x81
#define SETDISPLAYCLOCKDIV 0xD5
#define SETDISPLAYOFFSET 0xD3
#define SETHIGHCOLUMN 0x10
#define SETLOWCOLUMN 0x00
#define SETMULTIPLEX 0xA8
#define SETPRECHARGE 0xD9
#define SETSEGMENTREMAP 0xA1
#define SETSTARTLINE 0x40
#define SETVCOMDETECT 0xDB
#define SWITCHCAPVCC 0x2

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#define _swap_uint16_t(a, b) { uint16_t t = a; a = b; b = t; }

#endif

enum DISPLAY_COLOR {
  BLACK = 0,
  WHITE = 1,
  INVERSE = 2
};

enum DISPLAY_TYPE {
  OLED = 0,
  E_INK = 1
};

enum DISPLAY_TEXT_ALIGNMENT {
  TEXT_ALIGN_LEFT = 0,
  TEXT_ALIGN_RIGHT = 1,
  TEXT_ALIGN_CENTER = 2,
  TEXT_ALIGN_CENTER_BOTH = 3
};


enum DISPLAY_GEOMETRY {
  GEOMETRY_128_64   = 0,
  GEOMETRY_128_32,
  GEOMETRY_200_200,
  GEOMETRY_250_122,
  GEOMETRY_296_128,
  GEOMETRY_RAWMODE,
  GEOMETRY_64_32,
};


enum DISPLAY_ANGLE {
  ANGLE_0_DEGREE = 0,
  ANGLE_90_DEGREE,
  ANGLE_180_DEGREE,
  ANGLE_270_DEGREE,
};


typedef char (*FontTableLookupFunction)(const uint8_t ch);
char DefaultFontTableLookup(const uint8_t ch);


class ScreenDisplay : public Print  {
  public:
	ScreenDisplay();
    virtual ~ScreenDisplay();

	uint16_t width(void) const { return displayWidth; };
	uint16_t height(void) const { return displayHeight; };

    // Initialize the display
    bool init();

    // Free the memory used by the display
    void end();
    void sleep();

    void wakeup();

    // Cycle through the initialization
    void resetDisplay(void);

    /* Drawing functions */
    // Sets the color of all pixel operations
    void setColor(DISPLAY_COLOR color);

    // Returns the current color.
    DISPLAY_COLOR getColor();

    // Draw a pixel at given position
    void setPixel(int16_t x, int16_t y);

    // Draw a pixel at given position and color
    void setPixelColor(int16_t x, int16_t y, DISPLAY_COLOR color);

    // Clear a pixel at given position FIXME: INVERSE is untested with this function
    void clearPixel(int16_t x, int16_t y);

    // Draw a line from position 0 to position 1
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

    // Draw the border of a rectangle at the given location
    void drawRect(int16_t x, int16_t y, int16_t width, int16_t height);

    // Fill the rectangle
    void fillRect(int16_t x, int16_t y, int16_t width, int16_t height);

    // Draw the border of a circle
    void drawCircle(int16_t x, int16_t y, int16_t radius);

    // Draw all Quadrants specified in the quads bit mask
    void drawCircleQuads(int16_t x0, int16_t y0, int16_t radius, uint8_t quads);

    // Fill circle
    void fillCircle(int16_t x, int16_t y, int16_t radius);

    // Draw a line horizontally
    void drawHorizontalLine(int16_t x, int16_t y, int16_t length);

    // Draw a line vertically
    void drawVerticalLine(int16_t x, int16_t y, int16_t length);

    // Draws a rounded progress bar with the outer dimensions given by width and height. Progress is
    // a unsigned byte value between 0 and 100
    void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress);

    // Draw a bitmap in the internal image format
    void drawFastImage(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *image);

    // Draw a XBM
    void drawXbm(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *xbm);

    // Draw icon 16x16 xbm format
    void drawIco16x16(int16_t x, int16_t y, const char *ico, bool inverse = false);

    /* Text functions */

    // Draws a string at the given location
    void drawString(int16_t x, int16_t y, String text);

    // Draws a String with a maximum width at the given location.
    // If the given String is wider than the specified width
    // The text will be wrapped to the next line at a space or dash
    void drawStringMaxWidth(int16_t x, int16_t y, uint16_t maxLineWidth, String text);

    // Returns the width of the const char* with the current
    // font settings
    uint16_t getStringWidth(const char* text, uint16_t length);

    // Convencience method for the const char version
    uint16_t getStringWidth(String text);

    // Specifies relative to which anchor point
    // the text is rendered. Available constants:
    // TEXT_ALIGN_LEFT, TEXT_ALIGN_CENTER, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER_BOTH
    void setTextAlignment(DISPLAY_TEXT_ALIGNMENT textAlignment);

    // Sets the current font. Available default fonts
    // ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
    void setFont(const uint8_t *fontData);

    // Set the function that will convert utf-8 to font table index
    void setFontTableLookupFunction(FontTableLookupFunction function);

    /* Display functions */

    // Turn the display on
    void displayOn(void);

    // Turn the display offs
    void displayOff(void);

    // Inverted display mode
    void invertDisplay(void);

    // Normal display mode
    void normalDisplay(void);

    // Set display contrast
    // really low brightness & contrast: contrast = 10, precharge = 5, comdetect = 0
    // normal brightness & contrast:  contrast = 100
    void setContrast(uint8_t contrast, uint8_t precharge = 241, uint8_t comdetect = 64);

    // Convenience method to access 
    void setBrightness(uint8_t);

    // Reset display rotation or mirroring
    void resetOrientation();

    void screenRotate(DISPLAY_ANGLE angle);
    void resetScreenRotate();
    // Turn the display upside down
    void flipScreenVertically();

    // Write the buffer to the display memory
    virtual void display(void) = 0;

    // Clear the local pixel buffer
    void clear(void);

    // Log buffer implementation

    // This will define the lines and characters you can
    // print to the screen. When you exeed the buffer size (lines * chars)
    // the output may be truncated due to the size constraint.
    bool setLogBuffer(uint16_t lines, uint16_t chars);

    // Draw the log buffer at position (x, y)
    void drawLogBuffer(uint16_t x, uint16_t y);

    // Get screen geometry
    uint16_t getWidth(void);
    uint16_t getHeight(void);

    // Implement needed function to be compatible with Print class
    size_t write(uint8_t c);
    size_t write(const char* s);
	
    // Implement needed function to be compatible with Stream clas


    uint8_t            *buffer;
    #ifdef DISPLAY_DOUBLE_BUFFER
    uint8_t            *buffer_back;
    #endif

  protected:

    DISPLAY_GEOMETRY geometry;

    int8_t   rst=-1;
    uint16_t  displayWidth;
    uint16_t  displayHeight;
    uint16_t  displayBufferSize;
    DISPLAY_ANGLE rotate_angle = ANGLE_0_DEGREE;
    // Set the correct height, width and buffer for the geometry
    void setGeometry(DISPLAY_GEOMETRY g, uint16_t width = 0, uint16_t height = 0);
    void setRst(int8_t rst);
    
    DISPLAY_TEXT_ALIGNMENT   textAlignment;
    DISPLAY_COLOR            color;
    DISPLAY_TYPE             displayType;
    const uint8_t	 *fontData;

    // State values for logBuffer
    uint16_t   logBufferSize;
    uint16_t   logBufferFilled;
    uint16_t   logBufferLine;
    uint16_t   logBufferMaxLines;
    char      *logBuffer;

	// the header size of the buffer used, e.g. for the SPI command header
	virtual int getBufferOffset(void) = 0;
	
    // Send a command to the display (low level function)
    virtual void sendCommand(uint8_t com) {(void)com;};

    virtual void sendInitCommands(){};
    virtual void sendScreenRotateCommand(){};
    virtual void mirrorScreen(){}
    // Connect to the display
    virtual bool connect() { return false; };

    // Send all the init commands
    //void sendInitCommands();

    // converts utf8 characters to extended ascii
    char* utf8ascii(String s);

    void inline drawInternal(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const uint8_t *data, uint16_t offset, uint16_t bytesInData) __attribute__((always_inline));

    void drawStringInternal(int16_t xMove, int16_t yMove, char* text, uint16_t textLength, uint16_t textWidth);
	
	FontTableLookupFunction fontTableLookupFunction;
};

#endif

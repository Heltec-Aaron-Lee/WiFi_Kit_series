#ifndef OLED_H
#define OLED_H

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

// Uncomment to enable printing out nice debug messages.
//#define OLED_DEBUG

// Define where debug output will be printed.
#define DEBUG_PRINTER Serial

// Setup debug printing macros.
#ifdef OLED_DEBUG
	#define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
	#define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
	#define DEBUG_PRINT(...) {}
	#define DEBUG_PRINTLN(...) {}
#endif

class OLED {
	public:
		OLED(uint8_t sda, uint8_t scl, uint8_t address=0x3c, uint8_t offset=0);
		void begin(void);
		void on(void);
		void off(void);
		void clear(void);
		void print(char *s, uint8_t r=0, uint8_t c=0);

	private:
		uint8_t _sda, _scl, _address, _offset;
		void reset_display(void);
		void displayOn(void);
		void displayOff(void);
		void clear_display(void);
		void SendChar(unsigned char data);
		void sendCharXY(unsigned char data, int X, int Y);
		void sendcommand(unsigned char com);
		void setXY(unsigned char row,unsigned char col);
		void sendStr(unsigned char *string);
		void sendStrXY( const char *string, int X, int Y);
		void init_OLED(void);
};

#endif

/* vim: set ai et ts=4 sw=4: */
#ifndef __HT_ST7735_H__
#define __HT_ST7735_H__
#include <SPI.h>
#include <stdbool.h>
#include "HT_st7735_fonts.h"
#include "Arduino.h"

#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MV  0x20
#define ST7735_MADCTL_ML  0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH  0x04

/*** Redefine if necessary ***/
#define ST7735_CS_Pin        38
#define ST7735_REST_Pin      39
#define ST7735_DC_Pin        40
#define ST7735_SCLK_Pin      41
#define ST7735_MOSI_Pin      42
#define ST7735_LED_K_Pin     21
#define ST7735_VTFT_CTRL_Pin  3


// AliExpress/eBay 1.8" display, default orientation
/*
#define ST7735_IS_160X128 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160
#define ST7735_XSTART 0
#define ST7735_YSTART 0
#define ST7735_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MY)
*/

// AliExpress/eBay 1.8" display, rotate right
/*
#define ST7735_IS_160X128 1
#define ST7735_WIDTH  160
#define ST7735_HEIGHT 128
#define ST7735_XSTART 0
#define ST7735_YSTART 0
#define ST7735_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV)
*/

// AliExpress/eBay 1.8" display, rotate left
/*
#define ST7735_IS_160X128 1
#define ST7735_WIDTH  160
#define ST7735_HEIGHT 128
#define ST7735_XSTART 0
#define ST7735_YSTART 0
#define ST7735_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MV)
*/

// AliExpress/eBay 1.8" display, upside down
/*
#define ST7735_IS_160X128 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160
#define ST7735_XSTART 0
#define ST7735_YSTART 0
#define ST7735_ROTATION (0)
*/

// WaveShare ST7735S-based 1.8" display, default orientation
/*
#define ST7735_IS_160X128 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160
#define ST7735_XSTART 2
#define ST7735_YSTART 1
#define ST7735_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB)
*/

// WaveShare ST7735S-based 1.8" display, rotate right
/*
#define ST7735_IS_160X128 1
#define ST7735_WIDTH  160
#define ST7735_HEIGHT 128
#define ST7735_XSTART 1
#define ST7735_YSTART 2
#define ST7735_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_RGB)
*/

// WaveShare ST7735S-based 1.8" display, rotate left
/*
#define ST7735_IS_160X128 1
#define ST7735_WIDTH  160
#define ST7735_HEIGHT 128
#define ST7735_XSTART 1
#define ST7735_YSTART 2
#define ST7735_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_RGB)
*/

// WaveShare ST7735S-based 1.8" display, upside down
/*
#define ST7735_IS_160X128 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160
#define ST7735_XSTART 2
#define ST7735_YSTART 1
#define ST7735_ROTATION (ST7735_MADCTL_RGB)
*/

// 1.44" display, default orientation
/*
#define ST7735_IS_128X128 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 128
#define ST7735_XSTART 2
#define ST7735_YSTART 3
#define ST7735_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR)
*/

// 1.44" display, rotate right
/*
#define ST7735_IS_128X128 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 128
#define ST7735_XSTART 3
#define ST7735_YSTART 2
#define ST7735_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR)
*/

// 1.44" display, rotate left
/*
#define ST7735_IS_128X128 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 128
#define ST7735_XSTART 1
#define ST7735_YSTART 2
#define ST7735_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_BGR)
*/

// 1.44" display, upside down
/*
#define ST7735_IS_128X128 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 128
#define ST7735_XSTART 2
#define ST7735_YSTART 1
#define ST7735_ROTATION (ST7735_MADCTL_BGR)
*/

// mini 160x80 display (it's unlikely you want the default orientation)
/*
#define ST7735_IS_160X80 1
#define ST7735_XSTART 26
#define ST7735_YSTART 1
#define ST7735_WIDTH  80
#define ST7735_HEIGHT 160 
#define ST7735_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR)
*/

// mini 160x80, rotate left

#define ST7735_IS_160X80 1
#define ST7735_XSTART 1
#define ST7735_YSTART 26
#define ST7735_WIDTH  160
#define ST7735_HEIGHT 80
#define ST7735_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR)


// mini 160x80, rotate right 

// #define ST7735_IS_160X80 1
// #define ST7735_XSTART 1
// #define ST7735_YSTART 26
// #define ST7735_WIDTH  160
// #define ST7735_HEIGHT 80
// #define ST7735_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR)


/****************************/

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_GAMSET  0x26
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Color definitions
#define	ST7735_BLACK   0x0000
#define	ST7735_BLUE    0x001F
#define	ST7735_RED     0xF800
#define	ST7735_GREEN   0x07E0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW  0xFFE0
#define ST7735_WHITE   0xFFFF
#define ST7735_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

typedef enum {
	GAMMA_10 = 0x01,
	GAMMA_25 = 0x02,
	GAMMA_22 = 0x04,
	GAMMA_18 = 0x08
} GammaDef;


// call before initializing any SPI devices

class HT_st7735
{
private:
	/* data */
	void st7735_select(void);
	void st7735_unselect(void);
	void st7735_reset(void);
	void st7735_write_cmd(uint8_t cmd);
	void st7735_write_data(uint8_t* buff, size_t buff_size);
	void st7735_execute_cmd_list(const uint8_t *addr);
	void st7735_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
	int8_t 	  _cs_pin;
	int8_t    _rest_pin;     
	int8_t    _dc_pin;    
	int8_t    _sclk_pin;   
	int8_t    _mosi_pin;  
	int8_t    _led_k_pin; 
	int8_t    _vtft_ctrl_pin;
	uint16_t _width;
	uint16_t _height;
	uint16_t _x_start;
	uint16_t _y_start;
public:
	HT_st7735(	int8_t cs_pin=ST7735_CS_Pin,
				int8_t    rest_pin=ST7735_REST_Pin,     
				int8_t    dc_pin=ST7735_DC_Pin,    
				int8_t    sclk_pin=ST7735_SCLK_Pin,   
				int8_t    mosi_pin=ST7735_MOSI_Pin,  
				int8_t    led_k_pin=ST7735_LED_K_Pin,
				int8_t    vtft_ctrl_pin=ST7735_VTFT_CTRL_Pin );
	~HT_st7735();
	void st7735_init(void);
	void st7735_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
	void st7735_write_char(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor);
	void st7735_write_str(uint16_t x, uint16_t y, String str_data, FontDef font=Font_11x18, uint16_t color=ST7735_BLUE, uint16_t bgcolor=ST7735_BLACK);
	void st7735_write_str(uint16_t x, uint16_t y, const char *str, FontDef font=Font_11x18, uint16_t color=ST7735_BLUE, uint16_t bgcolor=ST7735_BLACK);
	void st7735_fill_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
	void st7735_fill_screen(uint16_t color);
	void st7735_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);
	void st7735_invert_colors(bool invert);
	void st7735_set_gamma(GammaDef gamma);
};


#endif // __ST7735_H__

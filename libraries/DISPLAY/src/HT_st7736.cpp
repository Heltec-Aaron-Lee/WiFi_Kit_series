/* vim: set ai et ts=4 sw=4: */

#include "HT_st7736.h"
#include "malloc.h"
#include "string.h"

SPIClass st7736_spi(HSPI);

// based on Adafruit ST7735 library for Arduino
#define DELAY 0x80

	const uint8_t  init_cmds1[] = {            // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      ST7735_ROTATION,        //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 };                 //     16-bit color

#if (defined(ST7735_IS_128X128) || defined(ST7735_IS_160X128))
  const uint8_t init_cmds2[] = {            // Init for 7735R, part 2 (1.44" display)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F };           //     XEND = 127
#endif // ST7735_IS_128X128
#ifdef ST7735_IS_160X80
  const uint8_t init_cmds2[] = {            // Init for 7735S, part 2 (160x80 display)
    3,                        //  3 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x4F,             //     XEND = 79
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F ,            //     XEND = 159
    ST7735_INVON, 0 };        //  3: Invert colors
#endif

  const uint8_t init_cmds3[] = {            // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Gamma Adjustments (pos. polarity), 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Gamma Adjustments (neg. polarity), 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100   //     100 ms delay
};

HT_st7736::HT_st7736(	int8_t cs_pin,int8_t rest_pin, int8_t  dc_pin,    
	                    int8_t sclk_pin,int8_t mosi_pin, int8_t led_k_pin,
                      int8_t    vtft_ctrl_pin )
{
	_cs_pin = cs_pin;
	_rest_pin = rest_pin;     
	_dc_pin = dc_pin;    
	_sclk_pin = sclk_pin;   
	_mosi_pin =  mosi_pin;  
	_led_k_pin = led_k_pin; 
  _vtft_ctrl_pin = vtft_ctrl_pin;
  _width = ST7735_WIDTH;
  _height = ST7735_HEIGHT;
  _x_start = ST7735_XSTART;
	_y_start = ST7735_YSTART;
}

HT_st7736::~HT_st7736()
{

}

void HT_st7736::st7735_select(void) 
{
  digitalWrite(_cs_pin, LOW);
}

void HT_st7736::st7735_unselect(void)
{
  digitalWrite(_cs_pin, HIGH);
}

void HT_st7736::st7735_reset(void) 
{
  digitalWrite(_rest_pin, LOW);    
  delay(5);
  digitalWrite(_rest_pin, HIGH);    
}

void HT_st7736::st7735_write_cmd(uint8_t cmd)
{
    digitalWrite(_dc_pin, LOW);   
		st7736_spi.transfer(cmd);
}

void HT_st7736::st7735_write_data(uint8_t* buff, size_t buff_size) 
{
    digitalWrite(_dc_pin, HIGH); 
    st7736_spi.transfer(buff, buff_size);
}

void HT_st7736::st7735_execute_cmd_list(const uint8_t *addr)
{
    uint8_t numCommands, numArgs;
    uint16_t ms;

    numCommands = *addr++;
    while(numCommands--) {
        uint8_t cmd = *addr++;
        st7735_write_cmd(cmd);

        numArgs = *addr++;
        // If high bit set, delay follows args
        ms = numArgs & DELAY;
        numArgs &= ~DELAY;
        if(numArgs) {
            st7735_write_data((uint8_t*)addr, numArgs);
            addr += numArgs;
        }

        if(ms) {
            ms = *addr++;
            if(ms == 255) ms = 500;
            delay(ms);
        }
    }
}

void HT_st7736::st7735_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) 
{
    // column address set
    st7735_write_cmd(ST7735_CASET);
    uint8_t data[] = { 0x00, x0 + _x_start, 0x00, x1 + _x_start };
    st7735_write_data(data, sizeof(data));

    // row address set
    st7735_write_cmd(ST7735_RASET);
    data[1] = y0 + _y_start;
    data[3] = y1 + _y_start;
    st7735_write_data(data, sizeof(data));

    // write to RAM
    st7735_write_cmd(ST7735_RAMWR);
}

void HT_st7736::st7735_init(void) 
{
    if((_dc_pin <0 ) || (_cs_pin <0 ) || (_rest_pin <0 ) || (_led_k_pin <0 ))
    {
      printf("Pin error!\r\n");
      return;
    }
    
    if(_vtft_ctrl_pin >= 0)
    {
      pinMode(_vtft_ctrl_pin, OUTPUT);
      digitalWrite(_vtft_ctrl_pin, LOW); 
    }

	  pinMode(_dc_pin, OUTPUT);  
    pinMode(_cs_pin, OUTPUT);
    pinMode(_rest_pin, OUTPUT);

    pinMode(_led_k_pin, OUTPUT);
    digitalWrite(_led_k_pin, HIGH);

    st7736_spi.begin(_sclk_pin,-1,_mosi_pin);
    st7735_select();
    st7735_reset();
    st7735_execute_cmd_list(init_cmds1);
    st7735_execute_cmd_list(init_cmds2);
    st7735_execute_cmd_list(init_cmds3);
    st7735_unselect();
}

void HT_st7736::st7735_draw_pixel(uint16_t x, uint16_t y, uint16_t color) 
{
    if((x >= _width) || (y >= _height))
    {
        return;
    }

    st7735_select();

    st7735_set_address_window(x, y, x+1, y+1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    st7735_write_data(data, sizeof(data));

    st7735_unselect();
}

void HT_st7736::st7735_write_char(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) 
{
    uint32_t i, b, j;

    st7735_set_address_window(x, y, x+font.width-1, y+font.height-1);

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                uint8_t data[] = { color >> 8, color & 0xFF };
                st7735_write_data(data, sizeof(data));
            } else {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
                st7735_write_data(data, sizeof(data));
            }
        }
    }
}
void  HT_st7736::st7735_write_str(uint16_t x, uint16_t y, String str_data, FontDef font, uint16_t color, uint16_t bgcolor) 
{
    const char *str=str_data.c_str();
    st7735_write_str( x, y, str,  font,  color,  bgcolor);
}

void  HT_st7736::st7735_write_str(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor) 
{
    st7735_select();
    while(*str) {
        if(x + font.width >= _width) {
            x = 0;
            y += font.height;
            if(y + font.height >= _height) {
                break;
            }
            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }
        st7735_write_char(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

    st7735_unselect();
}

void HT_st7736::st7735_fill_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= _width) || (y >= _height)) return;
    if((x + w - 1) >= _width) w = _width - x;
    if((y + h - 1) >= _height) h = _height - y;

    st7735_select();
    st7735_set_address_window(x, y, x+w-1, y+h-1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    digitalWrite(ST7735_DC_Pin, HIGH); 
    for(y = h; y > 0; y--) 
    {
        for(x = w; x > 0; x--) 
        {
            st7736_spi.transfer(data, sizeof(data));
        }
    }

    st7735_unselect();
}


void HT_st7736::st7735_fill_screen(uint16_t color) 
{
    st7735_fill_rectangle(0, 0, _width, _height, color);
}


void HT_st7736::st7735_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) 
{
    if((x >= _width) || (y >= _height)) return;
    if((x + w - 1) >= _width) return;
    if((y + h - 1) >= _height) return;

    st7735_select();
    st7735_set_address_window(x, y, x+w-1, y+h-1);
    st7735_write_data((uint8_t*)data, sizeof(uint16_t)*w*h);
    st7735_unselect();
}

void HT_st7736::st7735_invert_colors(bool invert) 
{
    st7735_select();
    st7735_write_cmd(invert ? ST7735_INVON : ST7735_INVOFF);
    st7735_unselect();
}

void HT_st7736::st7735_set_gamma(GammaDef gamma)
{
  uint8_t data[1];
  data[0] = (uint8_t)gamma;
	st7735_select();
	st7735_write_cmd(ST7735_GAMSET);
	st7735_write_data(data, sizeof(data));
	st7735_unselect();
}

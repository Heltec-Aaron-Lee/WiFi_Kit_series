#ifndef SSD1306Spi_h
#define SSD1306Spi_h

#include <HT_Display.h>
#include <SPI.h>


class SSD1306Spi : public ScreenDisplay {
  private:
      uint8_t             _rst;
      uint8_t             _dc;
      int8_t              _cs;
      int8_t              _clk;
      int8_t              _mosi;
      int8_t              _miso;
      uint32_t            _freq;
      uint8_t             _spi_num;
      SPISettings         _spiSettings;

  public:
    SSD1306Spi(uint8_t _rst, uint8_t _dc, int8_t _cs,  int8_t _sck, int8_t _mosi,int8_t _miso,uint32_t _freq = 6000000, DISPLAY_GEOMETRY g = 
GEOMETRY_128_64) {
        setGeometry(g);
      this->_rst = _rst;
      this->_dc  = _dc;
      this->_cs  = _cs;
      this->_freq  = _freq;
      this->_clk = _sck;
      this->_mosi = _mosi;
      this->_miso = _miso;
      this->displayType = OLED;
      }

    bool connect(){
      pinMode(_dc, OUTPUT);
      pinMode(_rst, OUTPUT);
      pinMode(_cs, OUTPUT);
      digitalWrite(_cs, HIGH);

      SPI.begin (this->_clk,this->_miso,this->_mosi);
      _spiSettings._clock=this->_freq;
      // Pulse Reset low for 10ms
      digitalWrite(_rst, HIGH);
      delay(1);
      digitalWrite(_rst, LOW);
      delay(10);
      digitalWrite(_rst, HIGH);
      return true;
    }

	void display(void) {

		if(rotate_angle==ANGLE_0_DEGREE||rotate_angle==ANGLE_180_DEGREE)
		{
		#ifdef DISPLAY_DOUBLE_BUFFER
			uint8_t minBoundY = UINT8_MAX;
			uint8_t maxBoundY = 0;

			uint8_t minBoundX = UINT8_MAX;
			uint8_t maxBoundX = 0;

			uint8_t x, y;

			// Calculate the Y bounding box of changes
			// and copy buffer[pos] to buffer_back[pos];
			for (y = 0; y < (displayHeight / 8); y++) 
			{
				for (x = 0; x < displayWidth; x++) {
					uint16_t pos = x + y * displayWidth;
					if (buffer[pos] != buffer_back[pos]) 
					{
						minBoundY = _min(minBoundY, y);
						maxBoundY = _max(maxBoundY, y);
						minBoundX = _min(minBoundX, x);
						maxBoundX = _max(maxBoundX, x);
					}
					buffer_back[pos] = buffer[pos];
				}
				//yield();
			}

			// If the minBoundY wasn't updated
			// we can savely assume that buffer_back[pos] == buffer[pos]
			// holdes true for all values of pos
			if (minBoundY == UINT8_MAX) return;

			sendCommand(COLUMNADDR);
			sendCommand(minBoundX);
			sendCommand(maxBoundX);

			sendCommand(PAGEADDR);
			sendCommand(minBoundY);
			sendCommand(maxBoundY);

			digitalWrite(_dc, HIGH);   // data mode
			digitalWrite(_cs,LOW);
			SPI.beginTransaction(_spiSettings);
			for (y = minBoundY; y <= maxBoundY; y++) 
			{
				for (x = minBoundX; x <= maxBoundX; x++)
				{
					SPI.transfer(buffer[x + y * displayWidth]);
				}
				//yield();
			}
			SPI.endTransaction();
			digitalWrite(_cs,HIGH);
		#else
			// No double buffering
			sendCommand(COLUMNADDR);
			sendCommand(0x0);
			sendCommand(0x7F);

			sendCommand(PAGEADDR);
			sendCommand(0x0);

			if (geometry == GEOMETRY_128_64)
			{
				sendCommand(0x7);
			} 
			else if (geometry == GEOMETRY_128_32)
			{
				sendCommand(0x3);
			}

			digitalWrite(_dc, HIGH);   // data mode
			digitalWrite(_cs,LOW);
			SPI.beginTransaction(_spiSettings);
			for (uint16_t i=0; i<displayBufferSize; i++) 
			{
				SPI.transfer(buffer[i]);
				yield();
			}
			SPI.endTransaction();
			digitalWrite(_cs,HIGH);
		#endif
		}
		else
		{
			uint8_t buffer_rotate[displayBufferSize];
			memset(buffer_rotate,0,displayBufferSize);
			uint8_t temp;
			for(uint16_t i=0;i<this->width();i++)
			{
				for(uint16_t j=0;j<this->height();j++)
				{
					temp = buffer[(j>>3)*this->width()+i]>>(j&7)&0x01;
					buffer_rotate[(i>>3)*this->height()+j]|=(temp<<(i&7));
				}
			}
		#ifdef DISPLAY_DOUBLE_BUFFER
			uint8_t minBoundY = UINT8_MAX;
			uint8_t maxBoundY = 0;
			 
			uint8_t minBoundX = UINT8_MAX;
			uint8_t maxBoundX = 0;
			uint8_t x, y;
			 
			// Calculate the Y bounding box of changes
			// and copy buffer[pos] to buffer_back[pos];
			for (y = 0; y < (this->width() / 8); y++)
			{
				for (x = 0; x < this->height(); x++)
				{
					uint16_t pos = x + y * this->height();
					if (buffer_rotate[pos] != buffer_back[pos])
					{
						minBoundY = _min(minBoundY, y);
						maxBoundY = _max(maxBoundY, y);
						minBoundX = _min(minBoundX, x);
						maxBoundX = _max(maxBoundX, x);
					}
					buffer_back[pos] = buffer_rotate[pos];
				}
				yield();
			}
			
			sendCommand(COLUMNADDR);
			sendCommand(minBoundX);
			sendCommand(maxBoundX);

			sendCommand(PAGEADDR);
			sendCommand(minBoundY);
			sendCommand(maxBoundY);

			digitalWrite(_dc, HIGH);   // data mode
			digitalWrite(_cs,LOW);
			SPI.beginTransaction(_spiSettings);
			for (y = minBoundY; y <= maxBoundY; y++) 
			{
				for (x = minBoundX; x <= maxBoundX; x++)
				{
					SPI.transfer(buffer_rotate[x + y * displayHeight]);
				}
				yield();
			}
			SPI.endTransaction();
			digitalWrite(_cs,HIGH);
		#else
			// No double buffering
			sendCommand(COLUMNADDR);
			sendCommand(0x0);
			sendCommand(0x7F);

			sendCommand(PAGEADDR);
			sendCommand(0x0);

			if (geometry == GEOMETRY_128_64)
			{
				sendCommand(0x7);
			} 
			else if (geometry == GEOMETRY_128_32)
			{
				sendCommand(0x3);
			}

			digitalWrite(_dc, HIGH);   // data mode
			digitalWrite(_cs,LOW);
			SPI.beginTransaction(_spiSettings);
			for (uint16_t i=0; i<displayBufferSize; i++) 
			{
				SPI.transfer(buffer_rotate[i]);
				yield();
			}
			SPI.endTransaction();
			digitalWrite(_cs,HIGH);
		#endif
		}
	}
	
    void stop(){
       end();
    }

  private:
	int getBufferOffset(void) {
		return 0;
	}
    inline void sendCommand(uint8_t com) __attribute__((always_inline)){
      digitalWrite(_dc, LOW);
      digitalWrite(_cs,LOW);
      SPI.beginTransaction(_spiSettings);
      SPI.transfer(com);
      SPI.endTransaction();
      digitalWrite(_cs,HIGH);
    }

	void sendInitCommands(void) 
	{
		if (geometry == GEOMETRY_RAWMODE)
			return;
		sendCommand(DISPLAYOFF);
		sendCommand(SETDISPLAYCLOCKDIV);
		sendCommand(0xF0); // Increase speed of the display max ~96Hz
		sendCommand(SETMULTIPLEX);
		sendCommand(this->height() - 1);
		sendCommand(SETDISPLAYOFFSET);
		sendCommand(0x00);
		sendCommand(SETSTARTLINE);
		sendCommand(CHARGEPUMP);
		sendCommand(0x14);
		sendCommand(MEMORYMODE);
		sendCommand(0x00);
		sendCommand(SEGREMAP|0x01);
		sendCommand(COMSCANDEC);
		sendCommand(SETCOMPINS);

		if (geometry == GEOMETRY_128_64) {
		sendCommand(0x12);
		} else if (geometry == GEOMETRY_128_32) {
		sendCommand(0x02);
		}

		sendCommand(SETCONTRAST);

		if (geometry == GEOMETRY_128_64) {
		sendCommand(0xCF);
		} else if (geometry == GEOMETRY_128_32) {
		sendCommand(0x8F);
		}

		sendCommand(SETPRECHARGE);
		sendCommand(0xF1);
		sendCommand(SETVCOMDETECT); //0xDB, (additionally needed to lower the contrast)
		sendCommand(0x40);	        //0x40 default, to lower the contrast, put 0
		sendCommand(DISPLAYALLON_RESUME);
		sendCommand(NORMALDISPLAY);
		sendCommand(0x2e);            // stop scroll
		sendCommand(DISPLAYON);
	}
	void sendScreenRotateCommand(){
		switch(this->rotate_angle)
		{
			case ANGLE_0_DEGREE:
				sendCommand(SEGREMAP|0x01); // Set Segment Re-map	//a1 mirror
				sendCommand(0xc8); // Set Common scan direction
				break;
			case ANGLE_90_DEGREE:
				sendCommand(SEGREMAP); // Set Segment Re-map   //a1 mirror
				sendCommand(0xc8); // Set Common scan direction
				break;
			case ANGLE_180_DEGREE:
				sendCommand(SEGREMAP); // Set Segment Re-map   //a1 mirror
				sendCommand(0xc0); // Set Common scan direction
				break;
			case ANGLE_270_DEGREE:
				sendCommand(SEGREMAP|0x01); // Set Segment Re-map	//a1 mirror
				sendCommand(0xc0); // Set Common scan direction
				break;
			default:
				break;
		}
	}
};

#endif

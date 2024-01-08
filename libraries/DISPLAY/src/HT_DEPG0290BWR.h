#ifndef __DEPG0290BWR_H__
#define __DEPG0290BWR_H__

#include <HT_Display.h>
#include <SPI.h>

enum DISPLAY_BUFFER {
  BLACK_BUFFER = 0,
  COLOR_BUFFER = 1,
  
};


class DEPG0290BWR : public ScreenDisplay {
  private:
      uint8_t             _rst;
      uint8_t             _dc;
      int8_t              _cs;
      int8_t              _clk;
      int8_t              _mosi;
      int8_t              _miso;
      uint32_t            _freq;
      uint8_t             _spi_num;
      int8_t              _busy;
      uint8_t             _buf[4736];
      SPISettings         _spiSettings;

  public:
    DEPG0290BWR(uint8_t _rst, uint8_t _dc, int8_t _cs, int8_t _busy, int8_t _sck, int8_t _mosi,int8_t _miso,uint32_t _freq = 6000000, DISPLAY_GEOMETRY g = 
GEOMETRY_296_128) {
        setGeometry(g);
      this->_rst = _rst;
      this->_dc  = _dc;
      this->_cs  = _cs;
      this->_freq  = _freq;
      this->_clk = _sck;
      this->_mosi = _mosi;
      this->_miso = _miso;
      this->_busy = _busy;
      this->displayType = E_INK;
      }

    bool connect(){
      pinMode(_dc, OUTPUT);
      pinMode(_rst, OUTPUT);
      pinMode(_cs, OUTPUT);
      digitalWrite(_cs, HIGH);
      pinMode(_busy, INPUT);
      this->buffer = _buf;
      SPI.begin (this->_clk,this->_miso,this->_mosi);
      _spiSettings._clock=this->_freq;
      // Pulse Reset low for 10ms
      digitalWrite(_rst, HIGH);
      delay(100);
      digitalWrite(_rst, LOW);
      delay(100);
      digitalWrite(_rst, HIGH);
      return true;
    }

	void update(DISPLAY_BUFFER buffer)
	{
		if(buffer == BLACK_BUFFER)
			updateData(0x10);
		else if(buffer == COLOR_BUFFER)
			updateData(0x13);
	}

	void display()
	{
		sendCommand(0x12);
		WaitUntilIdle();
	}

	void updateData(uint8_t addr) {

		if(rotate_angle==ANGLE_0_DEGREE||rotate_angle==ANGLE_180_DEGREE)
		{
			int xmax=this->width();
			int ymax=this->height()>>3;

			sendCommand(addr);
			digitalWrite(_cs,LOW);
			if(rotate_angle==ANGLE_0_DEGREE)
			{
				SPI.beginTransaction(SPISettings(6000000, LSBFIRST, SPI_MODE0));
				for(int x=xmax-1;x>=0;x--)
				{
					for(int y=0;y<ymax;y++)
					{
						SPI.transfer(~buffer[x + y * xmax]);
					}
				}
			}
			else
			{
				SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE0));
				for(int x=0;x<xmax;x++)
				{
					for(int y=(ymax-1);y>=0;y--)
					{
						SPI.transfer(~buffer[x + y * xmax]);
					}
				}
			}
			digitalWrite(_cs,HIGH);
			SPI.endTransaction();
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
			sendCommand(addr);
			digitalWrite(_cs,LOW);
			int xmax=this->height();
			int ymax=this->width()>>3;
			if(rotate_angle==ANGLE_90_DEGREE)
			{
				SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE0));
				for(int x=xmax-1;x>=0;x--)
				{
					for(int y=(ymax-1);y>=0;y--)
					{
						SPI.transfer(~buffer_rotate[x + y * xmax]);
					}
				}
			}
			else
			{
				SPI.beginTransaction(SPISettings(6000000, LSBFIRST, SPI_MODE0));
				for(int x=0;x<xmax;x++)
				{
					for(int y=0;y<ymax;y++)
					{
						SPI.transfer(~buffer_rotate[x + y * xmax]);
					}
				}
			}
			SPI.endTransaction();
			digitalWrite(_cs,HIGH);
		}
	}

    void stop(){
       end();
    }

  private:
	int getBufferOffset(void) {
		return 0;
	}

	void WaitUntilIdle()
	{
		while(digitalRead(_busy)) {      //LOW: idle, HIGH: busy
			delay(1);
		}
		delay(100);
	}
    inline void sendCommand(uint8_t com) __attribute__((always_inline)){
      digitalWrite(_dc, LOW);
      digitalWrite(_cs,LOW);
      SPI.beginTransaction(_spiSettings);
      SPI.transfer(com);
      SPI.endTransaction();
      digitalWrite(_cs,HIGH);
      digitalWrite(_dc, HIGH);
    }
	void sendData(unsigned char data) {
	  digitalWrite(this->_cs, LOW);
	  SPI.transfer(data);
	  digitalWrite(this->_cs, HIGH);
	}

	void sendInitCommands(void) 
	{
		if (geometry == GEOMETRY_RAWMODE)
			return;
		WaitUntilIdle();
		sendCommand(0x12); // soft reset
		WaitUntilIdle();

		sendCommand(0x06);
		sendData (0x17);
		sendData (0x17);
		sendData (0x17);	
		sendCommand(0x04);
		
		WaitUntilIdle();
		
		sendCommand(0X00);
		sendData(0x0f);
		sendData(0x0d);		//VCOM to 0V fast
		
		sendCommand(0x61);
		sendData (0x80);
		sendData (0x01);
		sendData (0x28);
		
		sendCommand(0X50); 		//VCOM AND DATA INTERVAL SETTING			
		sendData(0x77);		//WBmode:VBDF 17|D7 VBDW 97 VBDB 57 	WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7

	}
	void sendScreenRotateCommand(){}
};

#endif

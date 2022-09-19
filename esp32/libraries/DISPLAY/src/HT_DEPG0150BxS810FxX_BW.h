#ifndef __DEPG0150BxS810FxX_BW_H__
#define __DEPG0150BxS810FxX_BW_H__

#include <HT_Display.h>
#include <SPI.h>


class DEPG0150BxS810FxX_BW : public ScreenDisplay {
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
      uint8_t             _bbf[5000];
      SPISettings         _spiSettings;

  public:
    DEPG0150BxS810FxX_BW(uint8_t _rst, uint8_t _dc, int8_t _cs, int8_t _busy, int8_t _sck, int8_t _mosi,int8_t _miso,uint32_t _freq = 6000000, DISPLAY_GEOMETRY g = 
GEOMETRY_200_200) {
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
      this->buffer = _bbf;
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

	void display(void) {

		if(rotate_angle==ANGLE_0_DEGREE||rotate_angle==ANGLE_180_DEGREE)
		{
			sendCommand(0x24);
			digitalWrite(_cs,LOW);
			int xmax=this->width();
			int ymax=this->height()>>3;
			if(rotate_angle==ANGLE_0_DEGREE)
			{
				SPI.beginTransaction(SPISettings(6000000, LSBFIRST, SPI_MODE0));
				for(int x=0;x<xmax;x++)
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
				for(int x=xmax-1;x>=0;x--)
				{
					for(int y=(ymax-1);y>=0;y--)
					{
						SPI.transfer(~buffer[x + y * xmax]);
					}
				}
			}
			SPI.endTransaction();
			digitalWrite(_cs,HIGH);

			sendCommand(0x20);
			WaitUntilIdle();
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
			sendCommand(0x24);
			digitalWrite(_cs,LOW);
			int xmax=this->height();
			int ymax=this->width()>>3;
			if(rotate_angle==ANGLE_90_DEGREE)
			{
				SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE0));
				for(int x=0;x<xmax;x++)
				{
					for(int y=ymax-1;y>=0;y--)
					{
						SPI.transfer(~buffer_rotate[x + y * xmax]);
					}
				}
			}
			else
			{
				SPI.beginTransaction(SPISettings(6000000, LSBFIRST, SPI_MODE0));
				for(int x=xmax-1;x>=0;x--)
				{
					for(int y=0;y<ymax;y++)
					{
						SPI.transfer(~buffer_rotate[x + y * xmax]);
					}
				}
			}
			SPI.endTransaction();
			digitalWrite(_cs,HIGH);
			
			sendCommand(0x20);
			WaitUntilIdle();

			WaitUntilIdle();
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

		
		sendCommand(0x74); //set analog block control       
		sendData(0x54);
		sendCommand(0x7E); //set digital block control          
		sendData(0x3B);

		//sendCommand(0x01); //Driver output control      
		//sendData(0x27);
		//sendData(0x01);
		//sendData(0x00);

		sendCommand(0x11); //data entry mode       
		sendData(0x01);
		
		sendCommand(0x44); //set Ram-X address start/end position	
		sendData(0x00);
		sendData(0x18);    //0x18-->(24+1)*8=200 
		
		sendCommand(0x45); //set Ram-Y address start/end position		   
		sendData(0xC7);   //0xC7-->(199+1)=200
		sendData(0x00);
		sendData(0x00);
		sendData(0x00); 
		
		sendCommand(0x3C); //BorderWavefrom
		sendData(0x01); 
		
		sendCommand(0x18); 
		sendData(0x80); 
		
		sendCommand(0x4E);	 // set RAM x address count to 0;
		sendData(0x00);
		sendCommand(0x4F);	 // set RAM y address count to 0xF9-->(249+1)=250;	  
		sendData(0xC7);
		sendData(0x00);
		WaitUntilIdle();
	}
	void sendScreenRotateCommand(){}
};

#endif

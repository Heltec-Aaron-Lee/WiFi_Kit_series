#ifndef SH1107Wire_h
#define SH1107Wire_h

#include <HT_Display.h>
#include <Wire.h>


class SH1107Wire : public ScreenDisplay {
  private:
      uint8_t             _address;
      int                 _sda;
      int                 _scl;
      uint32_t             _freq;
      bool                _doI2cAutoInit = false;

  public:
    SH1107Wire(uint8_t _address, uint32_t _freq, int sda,int scl, DISPLAY_GEOMETRY g = GEOMETRY_128_64,int8_t _rst=-1) {
      setGeometry(g);
      setRst(_rst);
      this->_address = _address;
      this->_freq = _freq;
      this->_sda = sda;
      this->_scl = scl;
      this->displayType = OLED;
    }

    bool connect() {
      Wire.begin(_sda,_scl,_freq);
      return true;
    }

    void display(void) {
      initI2cIfNeccesary();

      if(rotate_angle==ANGLE_0_DEGREE||rotate_angle==ANGLE_180_DEGREE)
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
		  for (y = 0; y < (this->width() / 8); y++) {
			for (x = 0; x < this->height(); x++) {
			 uint16_t pos = x + y * this->height();
			 if (buffer_rotate[pos] != buffer_back[pos]) {
			   minBoundY = _min(minBoundY, y);
			   maxBoundY = _max(maxBoundY, y);
			   minBoundX = _min(minBoundX, x);
			   maxBoundX = _max(maxBoundX, x);
			 }
			 buffer_back[pos] = buffer_rotate[pos];
		   }
		   //yield();
		  }

		
		  // If the minBoundY wasn't updated
		  // we can savely assume that buffer_back[pos] == buffer[pos]
		  // holdes true for all values of pos
		
		if (minBoundY == UINT8_MAX) return;
		uint8_t page_number,column_number;
		
		for(page_number=minBoundY;page_number<=maxBoundY;page_number++)
		{
			  sendCommand(0xb0+page_number); //set page
			  sendCommand(0x10+minBoundX/16); //set column base  0x10 0x11 0x12... 0x1F
			  sendCommand(minBoundX%16);//set column offset 0x00 - 0x0F
			  Wire.beginTransmission(_address);
			  Wire.write(0x40);
			  for(column_number=minBoundX;column_number<=maxBoundX;column_number++)
			  {
				 Wire.write(buffer_rotate[column_number + page_number * this->height()]);
			  }
			  Wire.endTransmission();
		}
	#else
      	uint8_t page_number,column_number;
	
		for(page_number=0;page_number<this->width()>>3;page_number++)
		{
			  sendCommand(0xb0+page_number); //set page
			  sendCommand(0x10); //set column base	0x10 0x11 0x12... 0x1F
			  sendCommand(0x00);//set column offset 0x00 - 0x0F
			  Wire.beginTransmission(_address);
			  Wire.write(0x40);
			  for(column_number=0;column_number<this->height();column_number++)
			  {
				 Wire.write(buffer_rotate[column_number + page_number * this->height()]);
			  }
			  Wire.endTransmission();
		}
	#endif
	}
	
    else
    {
	      #ifdef DISPLAY_DOUBLE_BUFFER
	        uint8_t minBoundY = UINT8_MAX;
	        uint8_t maxBoundY = 0;

	        uint8_t minBoundX = UINT8_MAX;
	        uint8_t maxBoundX = 0;
	        uint8_t x, y;

	        // Calculate the Y bounding box of changes
	        // and copy buffer[pos] to buffer_back[pos];
	        for (y = 0; y < (this->height() / 8); y++) {
	          for (x = 0; x < this->width(); x++) {
	           uint16_t pos = x + y * this->width();
	           if (buffer[pos] != buffer_back[pos]) {
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

	      //  if (minBoundY == UINT8_MAX) return;

	   uint8_t page_number,column_number;

	   for(page_number=minBoundY;page_number<=maxBoundY;page_number++)
	   {
	         sendCommand(0xb0+page_number); //set page
	         sendCommand(0x10+minBoundX/16); //set column base  0x10 0x11 0x12... 0x1F
	         sendCommand(minBoundX%16);//set column offset 0x00 - 0x0F
	         Wire.beginTransmission(_address);
	         Wire.write(0x40);
	         for(column_number=minBoundX;column_number<=maxBoundX;column_number++)
	         {
	            Wire.write(buffer[column_number + page_number * this->width()]);
	         }
	         Wire.endTransmission();
	   }
	      #else

			uint8_t page_number,column_number;
			
			for(page_number=0;page_number<displayHeight/8;page_number++)
			{
				  sendCommand(0xb0+page_number); //set page
				  sendCommand(0x10); //set column base  0x10 0x11 0x12... 0x1F
				  sendCommand(0x00);//set column offset 0x00 - 0x0F
				  Wire.beginTransmission(_address);
				  Wire.write(0x40);
				  for(column_number=0;column_number<displayWidth;column_number++)
				  {
					 Wire.write(buffer[column_number + page_number * this->width()]);
				  }
				  Wire.endTransmission();
			}

	      #endif
	  }

    }

    void setI2cAutoInit(bool doI2cAutoInit) {
      _doI2cAutoInit = doI2cAutoInit;
    }
    
    void stop(){
	  end();
	  Wire.end();
    }

  private:
	int getBufferOffset(void) {
		return 0;
	}
    inline void sendCommand(uint8_t command) __attribute__((always_inline)){
      initI2cIfNeccesary();
      Wire.beginTransmission(_address);
      Wire.write(0x00);
      Wire.write(command);
      Wire.endTransmission();
    }

    void initI2cIfNeccesary() {
      if (_doI2cAutoInit) {
      	Wire.begin(_sda,_scl,_freq);
      }
    }

	void sendInitCommands(void)
	{
		sendCommand(0xae); // Display OFF

		sendCommand(SEGREMAP|0x01); // Set Segment Re-map   //a1 mirror
		sendCommand(0xc0); // Set Common scan direction
		sendCommand(0xd3); // Set Display Offset
		sendCommand(0x60);

		sendCommand(0xa8); // Set Multiplex Ration
		sendCommand(0x3f);
		sendCommand(0xd5); // Set Frame Frequency
		sendCommand(0x51); // 104Hz
		sendCommand(0xdc); // Set Display Start Line
		sendCommand(0x00);
		sendCommand(0x20); // Set Page Addressing Mode
		sendCommand(0x81); // Set Contrast Control
		sendCommand(0x90);
		sendCommand(0xa4); // Set Entire Display OFF/ON
		sendCommand(0xa6); // Set Normal/Reverse Display
		sendCommand(0xad); // Set External VPP
		sendCommand(0x8a);
		sendCommand(0xd9); // Set Phase Leghth
		sendCommand(0x22);
		sendCommand(0xdb); // Set Vcomh voltage
		sendCommand(0x35);
		sendCommand(0xaf); //Display ON
	}

	void sendScreenRotateCommand(){
		switch(this->rotate_angle)
		{
			case ANGLE_0_DEGREE:
				sendCommand(SEGREMAP|0x01); // Set Segment Re-map	//a1 mirror
				sendCommand(0xc0); // Set Common scan direction
				sendCommand(0xd3); // Set Display Offset
				sendCommand(0x60);
				break;
			case ANGLE_90_DEGREE:
				sendCommand(SEGREMAP); // Set Segment Re-map   //a1 mirror
				sendCommand(0xc0); // Set Common scan direction
				sendCommand(0xd3); // Set Display Offset
				sendCommand(0x60);
				break;
			case ANGLE_180_DEGREE:
				sendCommand(SEGREMAP); // Set Segment Re-map   //a1 mirror
				sendCommand(0xc8); // Set Common scan direction
				sendCommand(0xd3); // Set Display Offset
				sendCommand(0x20);
				break;
			case ANGLE_270_DEGREE:
				sendCommand(SEGREMAP|0x01); // Set Segment Re-map	//a1 mirror
				sendCommand(0xc8); // Set Common scan direction
				sendCommand(0xd3); // Set Display Offset
				sendCommand(0x20);
				break;
			default:
				break;
		}
	}
};

#endif

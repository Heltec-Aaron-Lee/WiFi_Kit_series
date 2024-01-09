#include "Arduino.h"
#include "HT_st7735.h"
#include "HT_TinyGPS++.h"


TinyGPSPlus GPS;
HT_st7735 st7735;

#define VGNSS_CTRL 3


void GPS_test(void)
{
	pinMode(VGNSS_CTRL,OUTPUT);
	digitalWrite(VGNSS_CTRL,LOW);
	Serial1.begin(115200,SERIAL_8N1,33,34);    
	Serial.println("GPS_test");
	st7735.st7735_fill_screen(ST7735_BLACK);
	delay(100);
	st7735.st7735_write_str(0, 0, (String)"GPS_test");

	while(1)
	{
		if(Serial1.available()>0)
		{
			if(Serial1.peek()!='\n')
			{
				GPS.encode(Serial1.read());
			}
			else
			{
				Serial1.read();
				if(GPS.time.second()==0)
				{
					continue;
				}
				st7735.st7735_fill_screen(ST7735_BLACK);
				st7735.st7735_write_str(0, 0, (String)"GPS_test");
				String time_str = (String)GPS.time.hour() + ":" + (String)GPS.time.minute() + ":" + (String)GPS.time.second()+ ":"+(String)GPS.time.centisecond();
				st7735.st7735_write_str(0, 20, time_str);
				String latitude = "LAT: " + (String)GPS.location.lat();
				st7735.st7735_write_str(0, 40, latitude);
				String longitude  = "LON: "+  (String)GPS.location.lng();
				st7735.st7735_write_str(0, 60, longitude);

				
				delay(5000);
				while(Serial1.read()>0);
			}
		}
	}
}

void setup(){
  delay(100);
  st7735.st7735_init();
  GPS_test();
}

void loop(){
    delay(100);
}

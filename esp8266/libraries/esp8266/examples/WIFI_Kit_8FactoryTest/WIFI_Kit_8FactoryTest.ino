/*
 * HelTec Automation(TM) WIFI_Kit_8 factory test demo code.
 * 
 * This test code must work with u8g2 library
 * 
 * You can search "u8g2" in Arduino library mananger and install it
 * 
 * Follow functions had been tested:
 * 
 * - Serial port(test with baud rate 115200)
 * - WIFI join
 * - WIFI scan
 * - Basic OLED display function test
 * - And some other Arduino basic funstions
 * 
 * by Aaron.Lee from HelTec AutoMation, ChengDu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.cn
 * support@heltec.cn
 *
 * this project also realess in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "ESP8266WiFi.h"

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4);   // pin remapping with ESP8266 HW I2C

void WIFISetUp(void)
{
	WiFi.disconnect();
	delay(100);
	WiFi.mode(WIFI_STA);
	WiFi.begin("Your WIFI SSID here","your WIFI password here");
	delay(100);

	byte count = 0;
	while(WiFi.status() != WL_CONNECTED && count < 10)
	{
		count ++;
		delay(500);
		u8g2.drawStr(0, 15, "Connecting...");
		u8g2.sendBuffer();
	}

	if(WiFi.status() == WL_CONNECTED)
	{
		u8g2.drawStr(0, 24, "Connecting...OK.");
		u8g2.sendBuffer();
	//		delay(500);
	}
	else
	{
		u8g2.drawStr(0, 24, "Connecting...Failed");
		u8g2.sendBuffer();
		while(1);
	}
	u8g2.drawStr(0, 32, "WIFI Setup done");
	u8g2.sendBuffer();
	delay(300);
}

void setup(void)
{
	Serial.begin(115200);
	Serial.println("uart init done");

	u8g2.begin();

	u8g2.clearBuffer();
	u8g2.setFontDirection(0);
	u8g2.setFontMode(1);
	u8g2.setFont(u8g2_font_5x7_mr);
	u8g2.drawStr(0, 7, "OLED init done");
	u8g2.sendBuffer();

	WIFISetUp();
}

void loop(void)
{
	u8g2.clearBuffer();

	u8g2.setFont(u8g2_font_6x12_mr);
	u8g2.drawStr(0, 7, "scan start...");
	u8g2.sendBuffer();

	int n = WiFi.scanNetworks();

	u8g2.clear();
	u8g2.drawStr(0, 7, "Scan done:");
	u8g2.sendBuffer();

	if(n == 0)
	{
	  u8g2.drawStr(0, 7, "no networks found");
	  u8g2.sendBuffer();
	}
	else
	{
		u8g2.setCursor(0, 25);
		u8g2.print(n);
		u8g2.print(" network found");
		u8g2.sendBuffer();
	}
	delay(2000);
}

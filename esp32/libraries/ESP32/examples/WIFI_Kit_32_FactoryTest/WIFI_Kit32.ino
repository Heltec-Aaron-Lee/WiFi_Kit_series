/*
 * HelTec Automation(TM) WIFI_LoRa_32 factory test code, witch includ
 * follow functions:
 *
 * - Basic OLED function test;
 *
 * - Basic serial port test(in baud rate 115200);
 *
 * - LED blink test;
 *
 * - WIFI join and scan test;
 *
 * - LoRa Ping-Pong test(DIO0 -- GPIO26 interrup check the new incoming messages;
 *
 * - Timer test and some other Arduino basic functions.
 *
 *by Aaron.Lee from HelTec AutoMation, ChengDu, China
 *�ɶ��������Զ����Ƽ����޹�˾
 *www.heltec.cn
 *
 *this project also realess in GitHub:
 *https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/

#include <Wire.h>
#include "SSD1306.h"
#include "WiFi.h"
#include "images.h"

#define SDA      4
#define SCL     15
#define RSTOLED  16   //RST must be set by software

#define LED  25

SSD1306  display(0x3c, SDA, SCL, RSTOLED);

void logo(){
  display.clear();
  display.drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
  display.display();
}

void WIFISetUp(void)
{
	//WIFI��ʼ�� + ɨ����ʾ
	// Set WiFi to station mode and disconnect from an AP if it was previously connected
	WiFi.disconnect(true);
	delay(1000);
	WiFi.mode(WIFI_STA);
	WiFi.setAutoConnect(true);
	WiFi.begin("your WIFI SSID","your password");
	delay(100);

	byte count = 0;
	while(WiFi.status() != WL_CONNECTED && count < 10)
	{
		count ++;
		delay(500);
		display.drawString(0, 0, "Connecting...");
		display.display();
	}

	display.clear();
	if(WiFi.status() == WL_CONNECTED)
	{
		display.drawString(0, 0, "Connecting...OK.");
		display.display();
//		delay(500);
	}
	else
	{
		display.clear();
		display.drawString(0, 0, "Connecting...Failed");
		display.display();
//		while(1);
	}
	display.drawString(0, 10, "WIFI Setup done");
	display.display();
	delay(500);
}

void WIFIScan()
{
    display.drawString(0, 20, "Scan start...");
    display.display();

    int n = WiFi.scanNetworks();
    display.drawString(0, 30, "Scan done");
    display.display();
    delay(500);
    display.clear();

    if (n == 0)
    {
    	display.clear();
		display.drawString(0, 0, "no network found");
		display.display();
		while(1);
    }
    else
    {
		display.drawString(0, 0, (String)n);
		display.drawString(14, 0, "networks found:");
		display.display();
		delay(500);

		for (int i = 0; i < n; ++i) {
		  // Print SSID and RSSI for each network found
		  display.drawString(0, (i+1)*9,(String)(i + 1));
		  display.drawString(6, (i+1)*9, ":");
		  display.drawString(12,(i+1)*9, (String)(WiFi.SSID(i)));
		  display.drawString(90,(i+1)*9, " (");
		  display.drawString(98,(i+1)*9, (String)(WiFi.RSSI(i)));
		  display.drawString(114,(i+1)*9, ")");
		//            display.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
		  delay(10);
		  }
      }
  //    display.println("");

    display.display();
    delay(800);
    display.clear();
}

void setup()
{
	pinMode(LED,OUTPUT);
	digitalWrite(LED,HIGH);
	delay(1);

	display.init();
	display.flipScreenVertically();
	display.setFont(ArialMT_Plain_10);
	logo();
	delay(300);
	display.clear();

	WIFISetUp();

	Serial.begin(115200);
	while (!Serial);
	Serial.println("Uart is working");
}

void loop()
{
	WIFIScan();
	delay(2000);
}

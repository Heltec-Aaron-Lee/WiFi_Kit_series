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
 *成都惠利特自动化科技有限公司
 *www.heltec.cn
 *
 *this project also realess in GitHub:
 *https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include "SSD1306.h"
#include "WiFi.h"
#include "images.h"

String rssi = "RSSI --";
String packSize = "--";
String packet;

unsigned int counter = 0;

bool receiveflag = false; // software flag for LoRa receiver, received data makes it true.

long lastSendTime = 0;        // last send time
int interval = 1000;          // interval between sends

// Pin definetion of WIFI LoRa 32
// HelTec AutoMation 2017 support@heltec.cn
#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI0     26   // GPIO26 -- SX127x's IRQ(Interrupt Request)

#define BAND    868E6  //you can set band here directly,e.g. 868E6,915E6
#define PABOOST true

SSD1306 display(0x3c, 4, 15);

void logo(){
  display.clear();
  display.drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
  display.display();
}

void WIFISetUp(void)
{
	//WIFI初始化 + 扫描演示
	// Set WiFi to station mode and disconnect from an AP if it was previously connected
	WiFi.disconnect(true);
	delay(1000);
	WiFi.mode(WIFI_STA);
	WiFi.setAutoConnect(true);
	WiFi.begin("your WIFI SSID","your WIFI password");
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
		while(1);
	}
	display.drawString(0, 10, "WIFI Setup done");
	display.display();
	delay(500);
}

void WIFIScan(unsigned int value)
{
  unsigned int i;
  for(i=0;i<value;i++)
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
		while(1); //搜不到wifi就进入死循环，移交品保部检查问题
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
}

void setup()
{
	pinMode(25,OUTPUT);

	pinMode(16,OUTPUT);
	digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
	delay(50);
	digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

	display.init();
	display.flipScreenVertically();
	display.setFont(ArialMT_Plain_10);
	logo();
	delay(300);
	display.clear();

	WIFISetUp();
	WIFIScan(1);

	Serial.begin(115200);
	while (!Serial);
	Serial.println("Uart is working");

	SPI.begin(SCK,MISO,MOSI,SS);

	LoRa.setPins(SS,RST,DI0);

	if (!LoRa.begin(BAND,PABOOST))
	{
	  display.drawString(0,0,"LoRa initial failed");
	  display.display();
	  while (1);
	}
	display.drawString(0, 0, "LoRa Initial success!");
	display.display();
	delay(300);
	display.clear();

	// register the receive callback
	LoRa.onReceive(onReceive);

	// put the radio into receive mode
	LoRa.receive();
}

void loop()
{
	if(millis() - lastSendTime > interval)//waiting LoRa interrupt
	{
		LoRa.beginPacket();
		LoRa.print("hello ");
		LoRa.print(counter++);
		LoRa.endPacket();

		LoRa.receive();

		digitalWrite(25,HIGH);
		display.drawString(0, 50, "Packet " + (String)(counter-1) + " sent done");
		display.display();

		interval = random(1000) + 1000; //1~2 seconds
		lastSendTime = millis();

    display.clear();
	}
	if(receiveflag)
	{
		display.drawString(0, 0, "Received " + packSize + " packages:");
		display.drawString(0, 10, packet);
		display.drawString(0, 20, "With " + rssi);
		display.display();

		digitalWrite(25,LOW);

		receiveflag = false;
	}
	//delay(1000);
}

void onReceive(int packetSize)//LoRa receiver interrupt service
{
	//if (packetSize == 0) return;

	packet = "";
    packSize = String(packetSize,DEC);

    while (LoRa.available())
    {
    		packet += (char) LoRa.read();
    }

    Serial.println(packet);
    rssi = "RSSI: " + String(LoRa.packetRssi(), DEC);

    receiveflag = true;
}


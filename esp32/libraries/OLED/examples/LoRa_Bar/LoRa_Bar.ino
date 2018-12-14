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
 * by Aaron.Lee from HelTec AutoMation, ChengDu, China
 * 成都惠利特自动化科技有限格式
 * www.heltec.cn
 *
 * this project also realess in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
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
#define SDA      4
#define SCL     15
#define RSTOLED  16   //RST must be set by software
#define Light  25
#define V2  1

#ifdef V2 //WIFI Kit series V1 not support Vext control
  #define Vext  21
#endif

#define BAND    868E6  //you can set band here directly,e.g. 868E6,915E6
#define PABOOST true

SSD1306  display(0x3c, SDA, SCL, RSTOLED);
OLEDDisplayUi ui     ( &display );
void WIFISetUp(void)
{
	// Set WiFi to station mode and disconnect from an AP if it was previously connected
	WiFi.disconnect(true);
	delay(100);
	WiFi.mode(WIFI_STA);
	WiFi.setAutoConnect(true);
	/*WiFi.begin("Your WIFI SSID","Your password");
	delay(100);

	byte count = 0;
	while(WiFi.status() != WL_CONNECTED && count < 10)
	{
		count ++;
		delay(500);
	}
	display.drawString(32, 32, "WIFI Setup OK");
	display.display();
	delay(500);
 display.clear();*/
}

void WIFIScan(unsigned int value)
{
  unsigned int i;
  for(i=0;i<value;i++)
  {
    display.drawString(32, 32, "Scan start...");
    display.display();

    int n = WiFi.scanNetworks();
    display.drawString(32, 40, "Scan done");
    display.display();
    delay(500);
    display.clear();
	  display.drawString(32, 40, (String)n + " nets found");
		display.display();
		delay(5000);
    display.clear();
  }
}

void setup()
{
	pinMode(Light,OUTPUT);
	pinMode(Vext,OUTPUT);
	digitalWrite(Vext, LOW);    //// OLED USE Vext as power supply, must turn ON Vext before OLED init
	delay(50);

	display.init();
	display.flipScreenVertically();
	display.setFont(ArialMT_Plain_10);
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
	  display.drawString(32,40,"LoRa failed");
	  display.display();
	  while (1);
	}
	display.drawString(32, 40, "LoRa success");
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

		digitalWrite(Light,HIGH);
//		display.drawString(32, 54, (String)(counter-1) + " sent done");
    //display.setFont(ArialMT_Plain_16);
    //display.drawString(34, 30, "HelTec");
    //display.setFont(ArialMT_Plain_10);
    //display.drawString(34, 54, "AutoMation®");
    display->drawXbm(x + 34, y + 12, LoRa_Logo_width, LoRa_Logo_height, LoRa_Logo_bits);
		display.display();

		interval = random(1000) + 1000; //1~2 seconds
		lastSendTime = millis();

    display.clear();
	}
	if(receiveflag)
	{
		display.drawString(32,29, "Received " + packSize);
		display.drawString(32,38, packet);
		display.drawString(32,47, rssi);
		display.display();

		digitalWrite(Light,LOW);

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

/*
 * HelTec Automation(TM) Vext control example.
 *
 * Function summary:
 *
 * - Vext connected to 3.3V via a MOS-FET, the gate pin connected to GPIO21;
 *
 * - OLED display and PE4259(RF switch) use Vext as power supply;
 *
 * - WIFI Kit series V1 don't have Vext control function;
 *
 * HelTec AutoMation, Chengdu, China.
 * 成都惠利特自动化科技有限公司
 * www.heltec.cn
 * support@heltec.cn
 *
 * this project also release in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/

#include "Arduino.h"
#include <Wire.h>
#include "SSD1306.h"

#define SDA		4  //OLED SDA pin
#define SCL		15 //OLED SCL pin
#define RST		16 //OLED nRST pin
#define Vext		21 //Vext control pin, HIGH -- disable Vext; LOW -- enable Vext.

SSD1306  display(0x3c, SDA, SCL, RST);

void setup()
{
	pinMode(Vext,OUTPUT);
	Serial.begin(115200);

	digitalWrite(Vext,LOW);
	Serial.println("Turn ON Vext");//Vext turn on
	delay(1000);
	//OLED use Vext power supply, Vext must be turn ON before OLED initialition.
	display.init();

	display.flipScreenVertically();
	display.setFont(ArialMT_Plain_10);
	display.drawString(0, 0, "Hello, I'm happy today");
	display.display();
	delay(1000);
}

void loop()
{
	display.sleep();//OLED sleep
	digitalWrite(Vext,HIGH);
	Serial.println("Turn OFF Vext");
	delay(5000);

	digitalWrite(Vext,LOW);
	Serial.println("Turn ON Vext");
	display.wakeup();
	delay(5000);
}


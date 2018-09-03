/*
 * HelTec Automation(TM) Electricity detection example.
 *
 * Function summary:
 *
 * - Vext connected to 3.3V via a MOS-FET, the gate pin connected to GPIO21;
 *
 * - Battery power detection is achieved by detecting the voltage of GPIO13;
 *
 * - OLED display and PE4259(RF switch) use Vext as power supply;
 *
 * - WIFI Kit series V1 don't have Vext control function;
 *
 * HelTec AutoMation, Chengdu, China.
 * �ɶ��������Զ����Ƽ����޹�˾
 * www.heltec.cn
 * support@heltec.cn
 *
 * this project also release in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 * 
*/
#include "Arduino.h"
#include <Wire.h>
#include "SSD1306.h"

#define Vext 21

#define SDA      4  //OLED SDA pin
#define SCL     15 //OLED SCL pin
#define RST     16 //OLED nRST pin
#define Fbattery    3700  //The default battery is 3700mv when the battery is fully charged.

float XS = 0.00225;      //The returned reading is multiplied by this XS to get the battery voltage.
uint16_t MUL = 1000;
uint16_t MMUL = 100;
SSD1306  display(0x3c, SDA, SCL, RST);

void setup()
{
	pinMode(Vext, OUTPUT);
	digitalWrite(Vext, LOW);
	delay(1000);

	display.init();
	display.flipScreenVertically();
	display.setFont(ArialMT_Plain_10);
	display.drawString(0, 0, "OLED Start");
	display.display();
	delay(1000);
	display.clear();

	adcAttachPin(13);
	analogSetClockDiv(255); // 1338mS

	Serial.begin(115200);
}

void loop()
{
   uint16_t c  =  analogRead(13)*XS*MUL;
   //uint16_t d  =  (analogRead(13)*XS*MUL*MMUL)/Fbattery;
   Serial.println(analogRead(13));
   //Serial.println((String)d);
  // Serial.printf("%x",analogRead(13));
   display.drawString(0, 0, "Remaining battery still has:");
   display.drawString(0, 10, "VBAT:");
   display.drawString(35, 10, (String)c);
   display.drawString(60, 10, "(mV)");
  // display.drawString(90, 10, (String)d);
  // display.drawString(98, 10, ".";
  // display.drawString(107, 10, "%");
   display.display();
   delay(2000);
   display.clear();
}


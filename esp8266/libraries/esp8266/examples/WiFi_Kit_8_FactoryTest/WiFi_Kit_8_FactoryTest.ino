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

#include <Wire.h>
#include "SSD1306.h"
#include "ESP8266WiFi.h"

SSD1306  display(0x3c, SDA, SCL, OLED_RST, GEOMETRY_128_32);

void WIFISetUp(void)
{
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin("Your WIFI SSID","Your password");
  delay(100);

  byte count = 0;
  while(WiFi.status() != WL_CONNECTED && count < 10)
  {
    count ++;
    delay(500);
    display.clear();
    display.drawString(0, 0, "Connecting...");
    display.display();
  }

  if(WiFi.status() == WL_CONNECTED)
  {
    display.drawString(0, 8, "Connecting...OK.");
    display.display();
  //    delay(500);
  }
  else
  {
    display.drawString(0, 8, "Connecting...Failed");
    display.display();
    while(1);
  }
  display.drawString(0, 16, "WIFI Setup done");
  display.display();
  delay(300);
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println("uart init done");

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  
  display.drawString(0, 0, "OLED init done");
  display.display();

  WIFISetUp();
}

void loop(void)
{
  display.setFont(ArialMT_Plain_16);
  
  display.clear();
  display.drawString(0, 0, "scan start...");
  display.display();

  int n = WiFi.scanNetworks();

  display.clear();
  display.drawString(0, 0, "Scan done:");
  display.display();

  if(n == 0)
  {
    display.drawString(0, 0, "no networks found");
    display.display();
  }
  else
  {
    display.drawString(0, 16, String(n));
    display.drawString(15, 16, " network found");
    display.display();
  }
  delay(2000);
}

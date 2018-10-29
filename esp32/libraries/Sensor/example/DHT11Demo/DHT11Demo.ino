// This example just provide basic function test;
// For more informations, please vist www.heltec.cn or mail to support@heltec.cn
#include <Wire.h>  
#include "SSD1306.h" 
#include "dht11.h" 
dht11 DHT11;
#define SDA    4
#define SCL   15
#define RST   16 //RST must be set by software
#define DHT11PIN    22

#define V2  0

#ifdef V2 //WIFI Kit series V1 not support Vext control
  #define Vext  21
#endif

#define DISPLAY_HEIGHT 64
#define DISPLAY_WIDTH  128


SSD1306  display(0x3c, SDA, SCL, RST);


void setup() {
//
//
//  pinMode(Vext,OUTPUT);
//  digitalWrite(Vext, LOW);    // OLED USE Vext as power supply, must turn ON Vext before OLED init
//  delay(50);
  Serial.begin(115200);
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  delay(1000);
}

void loop() { 
      display.display();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      Serial.println("/n");
      
      int chk = DHT11.read(DHT11PIN);
      Serial.print("Read sensor: ");
      switch (chk)
      {
      case DHTLIB_OK: 
                Serial.println("OK"); 
                break;
      case DHTLIB_ERROR_CHECKSUM: 
                Serial.println("Checksum error"); 
                break;
      case DHTLIB_ERROR_TIMEOUT: 
                Serial.println("Time out error"); 
                break;
      default: 
                Serial.println("Unknown error"); 
                break;
      }
      Serial.print("Humidity (%): ");
      Serial.println((float)DHT11.humidity, 2);
      Serial.print("Temperature (oC): ");
      Serial.println((float)DHT11.temperature, 2);
      display.clear();
      display.drawString(0, 5,  "Temperature:");
      display.drawString(70, 10,  (String) DHT11.temperature);
      display.drawString(85, 10, "oC");
      display.drawString(0, 35,  "Humidity :");
      display.drawString(60, 40, (String)DHT11.humidity);
      display.drawString(80, 40,  "(%)");
      display.display();
      delay(2000);
}


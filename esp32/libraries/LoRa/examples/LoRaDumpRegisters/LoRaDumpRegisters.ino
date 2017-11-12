/*
  LoRa Register Dumper

  This examples shows how to inspect and output the LoRa radio's
  registers on the Serial interface

  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
  
  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/
#include <SPI.h>
#include <LoRa.h>

// Pin definetion of WIFI LoRa 32
// HelTec AutoMation 2017 support@heltec.cn 
#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI0     26   // GPIO26 -- SX127x's IRQ(Interrupt Request)

#define BAND    433E6  //you can set band here directly,e.g. 868E6,915E6
#define PABOOST true


void setup() {
  Serial.begin(115200);               // initialize serial
  while (!Serial);

  Serial.println("LoRa Dump Registers");

  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0); // CS, reset, IRQ pin

  if (!LoRa.begin(BAND,PABOOST)) {         
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                   // if failed, do nothing
  }

  LoRa.dumpRegisters(Serial);
}


void loop() {
}


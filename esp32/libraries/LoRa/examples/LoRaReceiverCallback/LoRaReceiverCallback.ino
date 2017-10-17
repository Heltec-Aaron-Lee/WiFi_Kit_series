// This example just provide basic LoRa function test;
// Not the LoRa's farthest distance or strongest interference immunity.
// For more informations, please vist www.heltec.cn or mail to support@heltec.cn

#include <SPI.h>
#include <LoRa.h>

// WIFI_LoRa_32 ports

// GPIO5  -- SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO27 -- SX1278's MOSI
// GPIO18 -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request)

#define SS      18
#define RST     14
#define DI0     26
#define BAND    433E6

void setup() {
  Serial.begin(115200);
  while (!Serial); //if just the the basic function, must connect to a computer

  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  
  Serial.println("LoRa Receiver Callback");

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // register the receive callback
  LoRa.onReceive(onReceive);

  // put the radio into receive mode
  LoRa.receive();
}

void loop() {
  // do nothing
}

void onReceive(int packetSize) {
  // received a packet
  Serial.print("Received packet '");

  // read packet
  for (int i = 0; i < packetSize; i++) {
    Serial.print((char)LoRa.read());
  }

  // print RSSI of packet
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());
}


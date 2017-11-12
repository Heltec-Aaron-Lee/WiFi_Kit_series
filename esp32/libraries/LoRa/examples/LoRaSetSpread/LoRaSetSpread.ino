/*
  LoRa Duplex communication with Spreading Factor

  Sends a message every half second, and polls continually
  for new incoming messages. Sets the LoRa radio's spreading factor.

  Spreading factor affects how far apart the radio's transmissions
  are, across the available bandwidth. Radios with different spreading
  factors will not receive each other's transmissions. This is one way you
  can filter out radios you want to ignore, without making an addressing scheme.

  Spreading factor affects reliability of transmission at high rates, however,
  so avoid a hugh spreading factor when you're sending continually.

  See the Semtech datasheet, http://www.semtech.com/images/datasheet/sx1276.pdf
  for more on Spreading Factor.

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


byte msgCount = 0;            // count of outgoing messages
int interval = 2000;          // interval between sends
long lastSendTime = 0;        // time of last packet send

void setup()
{
  Serial.begin(115200);                   // initialize serial
  while (!Serial);

  Serial.println("LoRa Duplex - Set spreading factor");

  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0); // set CS, reset, IRQ pin

  if (!LoRa.begin(BAND,PABOOST))
  {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  LoRa.setSpreadingFactor(8);           // ranges from 6-12,default 7 see API docs
  Serial.println("LoRa init succeeded.");
}

void loop()
{
  if (millis() - lastSendTime > interval)
  {
    String message = "HeLoRa World! ";   // send a message
    message += msgCount;
    sendMessage(message);
    Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;    // 2-3 seconds
    msgCount++;
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  String incoming = "";

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}


/* Heltec Automation LoRaWAN communication example
 *
 * Function:
 * 1. Upload node data to the server using the standard LoRaWAN protocol.
 * 2. Print the data received by LoRaWAN to the webpage.
 * 
 * Description:
 * 1. Communicate using LoRaWAN protocol.
 * 
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * this project also realess in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 * */

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <LoRaWan_APP.h>


const char* ssid = "Your_WiFi_Name";
const char* password = "Your_WiFi_Password";


/* OTAA para*/
uint8_t devEui[] = { 0x22, 0x32, 0x33, 0x00, 0x00, 0x88, 0x88, 0x02 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x66, 0x01 };

/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_C;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

static void prepareTxFrame( uint8_t port )
{
    appDataSize = 4;//AppDataSize max value is 64
    appData[0] = 'A';
    appData[1] = 'B';
    appData[2] = 'C';
    appData[3] = 'D';
}

String LoRa_data;
uint16_t num=0;
bool LoRaDownLink = false;
uint32_t LoRadonwlinkTime;
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  LoRa_data = "";
  Serial.printf("+REV DATA:%s,RXSIZE %d,PORT %d\r\n",mcpsIndication->RxSlot?"RXWIN2":"RXWIN1",mcpsIndication->BufferSize,mcpsIndication->Port);
  Serial.print("+REV DATA:");
  for(uint8_t i=0;i<mcpsIndication->BufferSize;i++)
  {
    Serial.printf("%d",mcpsIndication->Buffer[i]);
    LoRa_data = LoRa_data + (String)(char)mcpsIndication->Buffer[i];
  }
  LoRaDownLink = true;
  LoRadonwlinkTime = millis();
  num++;
  Serial.println();
  Serial.print(num);
  Serial.print(":");
  Serial.println(LoRa_data);
}

WebServer server(80);
String htmlS ="<html>\
  <head>\
    <title>Wireless_Bridge</title>\
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
  </head>\
  <style type=\"text/css\">\
    .header{display: block;margin-top:10px;text-align:center;width:100%;font-size:10px}\
    .input{display: block;text-align:center;width:100%}\
    .input input{height: 20px;width: 300px;text-align:center;border-radius:15px;}\
    .input select{height: 26px;width: 305px;text-align:center;border-radius:15px;}\
    .btn,button{width: 305px;height: 40px;border-radius:20px; background-color: #000000; border:0px; color:#ffffff; margin-top:20px;}\
  </style>\
  <script type=\"text/javascript\">\
    function myrefresh()\
    {\
      window.location.reload();\
    }\
window.onload=function(){\
      setTimeout('myrefresh()',10000);\
      }   \
  </script>\
  <body>\
    <div style=\"width:100%;text-align:center;font-size:25px;font-weight:bold;margin-bottom:20px\">Wireless_Bridge</div>\
      <div style=\"width:100%;text-align:center;\">\
        <div class=\"header\"><span>(Note 1: The default refresh time of this page is 10s. If you need to modify the refresh time, you can modify it in the 'setTimeout' function.)</span></div>\
        <div class=\"header\"><span>(Note 2: The refresh time needs to be modified according to the data sending frequency.)</span></div>\
        <div class=\"header\"><span>Data: ";
  String htmlF = "</span></div>\
      </form>\
    </div>\
  </body>\
</html>";

String htmlW = "<html>\
  <head>\
    <title>Wireless_Bridge</title>\
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
  </head>\
  <style type=\"text/css\">\
    .header{display: block;margin-top:10px;text-align:center;width:100%;font-size:10px}\
    .input{display: block;text-align:center;width:100%}\
    .input input{height: 20px;width: 300px;text-align:center;border-radius:15px;}\
    .input select{height: 26px;width: 305px;text-align:center;border-radius:15px;}\
    .btn,button{width: 305px;height: 40px;border-radius:20px; background-color: #000000; border:0px; color:#ffffff; margin-top:20px;}\
  </style>\
  <script type=\"text/javascript\">\
window.onload=function(){\
      document.getElementsByName(\"server\")[0].value = \"\";\
      }   \
  </script>\
  <body>\
    <div style=\"width:100%;text-align:center;font-size:25px;font-weight:bold;margin-bottom:20px\">Wireless_Bridge</div>\
      <div style=\"width:100%;text-align:center;\">\
        <div class=\"header\"><span>(Note 1: The data sent from this webpage to LoRa needs to be decoded to be able to be viewed normally.)</span></div>\
        <div class=\"header\"><span>(Note 2: The default size of data sent by LoRa is 4 bytes, which can be modified in the program.)</span></div>\
        <form method=\"POST\" action=\"\" onsubmit=\"\">\
          <div class=\"header\"><span>DATA</span></div>\
          <div class=\"input\"><input type=\"text\"  name=\"server\" value=\"\"></div>\
        <div class=\"header\"><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"Submit\"></div>\
      </form>\
    </div>\
  </body>\
</html>";

String Page_data="";
String symbol=":";
void ROOT_HTML()
{
  Page_data =Page_data+ (String)num+ symbol + LoRa_data +"<br>";
  String html = htmlS + Page_data + htmlF;
  server.send(200,"text/html",html);
}

bool WiFiDownLink = false;
uint32_t WiFidonwlinkTime;
String Write_data="";
void ROOT_HTMLW()
{
  if(server.hasArg("server"))
  {
    Serial.println(server.arg("server"));
    Write_data=server.arg("server");
    WiFiDownLink = true;
    WiFidonwlinkTime = millis();
    unsigned char *puc;
    puc = (unsigned char *)(&Write_data);
    appDataSize = 4;//AppDataSize max value is 64
    appData[0] = puc[0];
    appData[1] = puc[1];
    appData[2] = puc[2];
    appData[3] = puc[3];
    LoRaWAN.send();
  }
  server.send(200,"text/html",htmlW);
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial);
  Mcu.begin();
  deviceState = DEVICE_STATE_INIT;
  pinMode(WIFI_LED, OUTPUT);
  pinMode(LoRa_LED, OUTPUT);
  digitalWrite(WIFI_LED, HIGH);
  digitalWrite(LoRa_LED, HIGH);
  delay(1000);
  digitalWrite(WIFI_LED, LOW);
  digitalWrite(LoRa_LED, LOW);
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    server.on("/", ROOT_HTML);
    server.on("/w", ROOT_HTMLW);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("View page IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Write page IP address: ");
    Serial.print(WiFi.localIP());
    Serial.println("/w");
  } else {
    Serial.println("WiFi Failed");
  }
}

void loop(void) {
  server.handleClient();
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
            LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
  if(LoRaDownLink)
  {
    LoRaDownLink = false;
    digitalWrite(LoRa_LED,HIGH);
  }
  else if(digitalRead(LoRa_LED) && (millis()-LoRadonwlinkTime)> 1000)
  {
    digitalWrite(LoRa_LED,LOW);
  }
  if(WiFiDownLink)
  {
    WiFiDownLink = false;
    digitalWrite(WIFI_LED,HIGH);
  }
  else if(digitalRead(WIFI_LED) && (millis()-WiFidonwlinkTime)> 1000)
  {
    digitalWrite(WIFI_LED,LOW);
  }
}
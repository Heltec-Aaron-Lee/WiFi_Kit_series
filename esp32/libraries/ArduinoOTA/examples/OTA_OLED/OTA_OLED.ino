

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <Wire.h>
#include "SSD1306.h"


/**********************************************  WIFI Client 注意编译时要设置此值 *********************************
 * wifi client
 */
const char* ssid = "xxxxxx"; //replace "xxxxxx" with your WIFI's ssid
const char* password = "xxxxxx"; //replace "xxxxxx" with your WIFI's password

//WiFi&OTA 参数
//#define HOSTNAME "HelTec_OTA_OLED"
#define PASSWORD "HT.123456" //the password for OTA upgrade, can set it in any char you want

/************************************************  注意编译时要设置此值 *********************************
 * 是否使用静态IP
 */
#define USE_STATIC_IP false
#if USE_STATIC_IP
  IPAddress staticIP(192,168,1,22);
  IPAddress gateway(192,168,1,9);
  IPAddress subnet(255,255,255,0);
  IPAddress dns1(8, 8, 8, 8);
  IPAddress dns2(114,114,114,114);
#endif

/*******************************************************************
 * OLED Arguments
 */
#define RST_OLED 16                     //OLED Reset引脚，需要手动Reset，否则不显示
#define OLED_UPDATE_INTERVAL 500        //OLED屏幕刷新间隔ms
SSD1306 display(0x3C, 4, 15);           //引脚4，15是绑定在Kit 32的主板上的，不能做其它用


/********************************************************************
 * OTA升级配置
 */
void setupOTA()
{
  //Port defaults to 8266
  //ArduinoOTA.setPort(8266);

  //Hostname defaults to esp8266-[ChipID]
//  ArduinoOTA.setHostname(HOSTNAME);

  //No authentication by default
  ArduinoOTA.setPassword(PASSWORD);

  //Password can be set with it's md5 value as well
  //MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
  {
    display.clear();
    display.setFont(ArialMT_Plain_10);        //设置字体大小
    display.setTextAlignment(TEXT_ALIGN_LEFT);//设置字体对齐方式
    display.drawString(0, 0, "Start Updating....");

    Serial.printf("Start Updating....Type:%s\n", (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem");
  });

  ArduinoOTA.onEnd([]()
  {
    display.clear();
    display.drawString(0, 0, "Update Complete!");
    Serial.println("Update Complete!");

    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    String pro = String(progress / (total / 100)) + "%";
    int progressbar = (progress / (total / 100));
    //int progressbar = (progress / 5) % 100;
    //int pro = progress / (total / 100);

    display.clear();
    display.drawProgressBar(0, 32, 120, 10, progressbar);    // draw the progress bar    
    display.setTextAlignment(TEXT_ALIGN_CENTER);          // draw the percentage as String
    display.drawString(64, 15, pro);
    display.display();

    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    Serial.printf("Error[%u]: ", error);
    String info = "Error Info:";
    switch(error)
    {
      case OTA_AUTH_ERROR:
        info += "Auth Failed";
        Serial.println("Auth Failed");
        break;

      case OTA_BEGIN_ERROR:
        info += "Begin Failed";
        Serial.println("Begin Failed");        
        break;

      case OTA_CONNECT_ERROR:
        info += "Connect Failed";
        Serial.println("Connect Failed");        
        break;

      case OTA_RECEIVE_ERROR:
        info += "Receive Failed";
        Serial.println("Receive Failed");        
        break;

      case OTA_END_ERROR:
        info += "End Failed";
        Serial.println("End Failed");        
        break;
    }

    display.clear();
    display.drawString(0, 0, info);    
    ESP.restart();
  });

  ArduinoOTA.begin();
}

/********************************************************************
 * setup oled
 */
void setupOLED()
{
  pinMode(RST_OLED, OUTPUT);
  //复位OLED电路
  digitalWrite(RST_OLED, LOW);        // turn D16 low to reset OLED
  delay(50);
  digitalWrite(RST_OLED, HIGH);       // while OLED is running, must set D16 in high
  
  display.init();
  display.flipScreenVertically();           //倒过来显示内容
  display.setFont(ArialMT_Plain_10);        //设置字体大小
  display.setTextAlignment(TEXT_ALIGN_LEFT);//设置字体对齐方式

  display.clear();
  display.drawString(0, 0, "Initialize...");
}

/*********************************************************************
 * setup wifi
 */
void setupWIFI()
{
  display.clear();
  display.drawString(0, 0, "Connecting...");
  display.drawString(0, 10, String(ssid));
  display.display();
  
  //连接WiFi，删除旧的配置，关闭WIFI，准备重新配置
  WiFi.disconnect(true);
  delay(1000);
  
  WiFi.mode(WIFI_STA);  
  //WiFi.onEvent(WiFiEvent); 
  WiFi.setAutoConnect(true);      
  WiFi.setAutoReconnect(true);    //断开WiFi后自动重新连接,ESP32不可用
  //WiFi.setHostname(HOSTNAME);
  WiFi.begin(ssid, password);
#if USE_STATIC_IP
  WiFi.config(staticIP, gateway, subnet);
#endif

  //等待5000ms，如果没有连接上，就继续往下
  //不然基本功能不可用
  byte count = 0;
  while(WiFi.status() != WL_CONNECTED && count < 10)
  {
    count ++;
    delay(500);
    Serial.print(".");
  }

  display.clear();
  if(WiFi.status() == WL_CONNECTED)
    display.drawString(0, 0, "Connecting...OK."); 
  else
    display.drawString(0, 0, "Connecting...Failed");
  display.display();
}

/******************************************************
 * arduino setup
 */
void setup() 
{
  pinMode(25, OUTPUT);
  digitalWrite(25,HIGH);
  
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Initialize...");
  
  setupOLED();
  setupWIFI();
  setupOTA();
}

/******************************************************
 * arduino loop
 */
void loop() 
{
  ArduinoOTA.handle();
  unsigned long ms = millis();
  if(ms % 1000 == 0)
  {
    Serial.println("hello，OTA now");
  }
}

/****************************************************
 * [通用函数]ESP32 WiFi Kit 32事件处理
 */
void WiFiEvent(WiFiEvent_t event) 
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) 
    {
        case SYSTEM_EVENT_WIFI_READY:               /**< ESP32 WiFi ready */
            break;
        case SYSTEM_EVENT_SCAN_DONE:                /**< ESP32 finish scanning AP */
            break;
            
        case SYSTEM_EVENT_STA_START:                /**< ESP32 station start */
            break;
        case SYSTEM_EVENT_STA_STOP:                 /**< ESP32 station stop */
            break;
            
        case SYSTEM_EVENT_STA_CONNECTED:            /**< ESP32 station connected to AP */
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:         /**< ESP32 station disconnected from AP */
            break;
            
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:      /**< the auth mode of AP connected by ESP32 station changed */
            break;
            
        case SYSTEM_EVENT_STA_GOT_IP:               /**< ESP32 station got IP from connected AP */
        case SYSTEM_EVENT_STA_LOST_IP:              /**< ESP32 station lost IP and the IP is reset to 0 */
            break;
            
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:       /**< ESP32 station wps succeeds in enrollee mode */
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:        /**< ESP32 station wps fails in enrollee mode */
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:       /**< ESP32 station wps timeout in enrollee mode */
        case SYSTEM_EVENT_STA_WPS_ER_PIN:           /**< ESP32 station wps pin code in enrollee mode */
            break;
            
        case SYSTEM_EVENT_AP_START:                 /**< ESP32 soft-AP start */
        case SYSTEM_EVENT_AP_STOP:                  /**< ESP32 soft-AP stop */
        case SYSTEM_EVENT_AP_STACONNECTED:          /**< a station connected to ESP32 soft-AP */
        case SYSTEM_EVENT_AP_STADISCONNECTED:       /**< a station disconnected from ESP32 soft-AP */
        case SYSTEM_EVENT_AP_PROBEREQRECVED:        /**< Receive probe request packet in soft-AP interface */
        case SYSTEM_EVENT_AP_STA_GOT_IP6:           /**< ESP32 station or ap interface v6IP addr is preferred */
            break;
            
        case SYSTEM_EVENT_ETH_START:                /**< ESP32 ethernet start */
        case SYSTEM_EVENT_ETH_STOP:                 /**< ESP32 ethernet stop */
        case SYSTEM_EVENT_ETH_CONNECTED:            /**< ESP32 ethernet phy link up */
        case SYSTEM_EVENT_ETH_DISCONNECTED:         /**< ESP32 ethernet phy link down */
        case SYSTEM_EVENT_ETH_GOT_IP:               /**< ESP32 ethernet got IP from connected AP */
        case SYSTEM_EVENT_MAX:
            break;
    }
}


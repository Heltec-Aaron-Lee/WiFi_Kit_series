/*
 * Hold GPIO status in deep sleep mode, use ESP32's ULP (Ultra Low Power) coprocessor
 *
 * HelTec AutoMation, Chengdu, China.
 * 成都惠利特自动化科技有限公司
 * www.heltec.cn
 * support@heltec.cn
 *
 *this project also release in GitHub:
 *https://github.com/HelTecAutomation/ESP32_LoRaWAN
*/
#include "soc/rtc_io_reg.h"
#include "driver/rtc_io.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

void setup(){
  rtc_gpio_hold_dis(GPIO_NUM_25);
  pinMode(25,OUTPUT);
  digitalWrite(25,LOW);//The on board LED will be OFF in wake up period
  
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  delay(2);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");
  delay(10);
  
  Serial.println("Going to sleep now");
  delay(2);

  rtc_gpio_init(GPIO_NUM_25);
  pinMode(25,OUTPUT);
  digitalWrite(25,HIGH);//The on board LED will be ON in deep sleep period
  rtc_gpio_hold_en(GPIO_NUM_25);
  
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop(){
  //This is not going to be called
}

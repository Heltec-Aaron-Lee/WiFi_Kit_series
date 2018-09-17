/*
 * HelTec Automation(TM) WIFI_LoRa_32 RTC test code.
 * 
 * Use internal 15KHz crystal, difference between adjacent two data
 * should close to 150000.
 * 
 *by Harry from HelTec AutoMation, ChengDu, China
 *成都惠利特自动化科技有限公司
 *www.heltec.cn
 *
 *this project also realess in GitHub:
 *https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/

#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"

void setup() {
	Serial.begin(115200);
  delay(3000);//RTC init and start up need a long time.
}

void loop() {
	Serial.println((uint32_t)rtc_time_get());
	delay(1000);
}

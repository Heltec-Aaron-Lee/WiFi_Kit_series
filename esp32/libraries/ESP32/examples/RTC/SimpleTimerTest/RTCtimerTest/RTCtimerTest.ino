/*
 * HelTec Automation(TM) WIFI_LoRa_32 RTC test code.
 * 
 * Use external 32.768KHz crystal, difference between adjacent two data
 * should close to 32768.
 * 
 *by Harry from HelTec AutoMation, ChengDu, China
 *成都惠利特自动化科技有限公司
 *www.heltec.cn
 *
 *this project also realess in GitHub:
 *https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/

#include "soc\rtc.h"
#include "soc\rtc_cntl_reg.h"
#include "driver/rtc_io.h"

void setup() {
	Serial.begin(115200);
	rtc_clk_32k_enable(true);
	rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
  delay(3000);//RTC init and start up need a long time.
}

void loop() {
	Serial.println((uint32_t)rtc_time_get());
	delay(1000);
}

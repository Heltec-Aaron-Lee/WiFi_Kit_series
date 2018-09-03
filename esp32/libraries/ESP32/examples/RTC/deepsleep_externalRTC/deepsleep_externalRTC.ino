#include "soc\rtc.h"
#include "soc\rtc_cntl_reg.h"
#include "driver/rtc_io.h"


RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint64_t RtcSwitch=0;
/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}

void setup(){
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor
  print_wakeup_reason();

  if (bootCount==0)
  {
	  //Initialize the external 32K XTAL,and compute the tick for esp_sleep_enable_timer_wakeup();
	  //compute the real frequence of Internal 150 kHz RC oscillator
	  uint32_t B=(uint32_t)rtc_time_get();
	  delay(1000);
	  uint32_t A=(uint32_t)rtc_time_get();
	  Serial.printf("Freq of Internal 150 kHz RC oscillator:%d\r\n",A-B);

	  //compute the sleep ticks  for 1S with external 32K XTAL
	  RtcSwitch=0X7A1200000/(A-B);//1000000*32768/(150K Êµ¼ÊÆµÂÊ)£»
	  Serial.printf("RtcSwitch:%d\r\n",(uint32_t)RtcSwitch);

	  //enable external 32K XTAL and switch RTC to external 32K XTAL
	  rtc_clk_32k_enable(true);
	  rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
	  //wait for external 32K XTAL start
	  Serial.printf("Waiting for external 32K XTAL start OK¡­¡­\r\n");
	  delay(3000);
  }
  else
  {
	  rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
  }

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.printf("bootCount:%d\r\n",bootCount);
  //set deepsleep time for 5s
  esp_sleep_enable_timer_wakeup(RtcSwitch*5);

  //Go to sleep now
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();

  //This will never be printed
  Serial.println("This will never be printed");
}

void loop(){
  //This is not going to be called
}

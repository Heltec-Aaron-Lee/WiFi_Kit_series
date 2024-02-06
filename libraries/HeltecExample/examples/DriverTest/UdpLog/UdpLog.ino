#include "Arduino.h"
#include "esp_network_log.h"
const char* ssid = "TP-LINK_666";
const char* password = "heltec_test";
#define TAG "UdpLOG"

int i = 0;
char buf_read[128];

void setup() {
	Serial.begin(115200);
	esp_network_log.init(ssid,password,(esp_network_log_level_t)NLOG_LOCAL_LEVEL,true);
}
void loop() {
  	printf("NLOG_LOCAL_LEVEL =%d\r\n",NLOG_LOCAL_LEVEL);
	ESP_NLOGI(TAG,"ESP32 Chip model Rev %d\n", i++);
    ESP_NLOGW(TAG,"ESP32 Chip model Rev %d\n", i++);
  	ESP_NLOGE(TAG,"ESP32 Chip model Rev %d\n", i++);

	if(esp_network_log.available())
	{
		int len = esp_network_log.read(buf_read,128);
		buf_read[len] = '\0';
		ESP_NLOGW(TAG,"%s \r\n", buf_read); 
	}
	delay(1000);
}
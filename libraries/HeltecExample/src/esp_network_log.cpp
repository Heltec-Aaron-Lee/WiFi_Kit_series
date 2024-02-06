#include "esp_network_log.h"
#include <WiFi.h>
#include "ESPmDNS.h"
#include <ArduinoOTA.h>

#define ESP_LOG_REDIRECTION_CALLOC calloc
#define ESP_LOG_REDIRECTION_MALLOC malloc
#define ESP_LOG_REDIRECTION_FREE   free
#define MDEBUG_LOG_TIMEOUT_MS      (1000)

#define UDP_LOG_PORT (12345)
#define TCP_OTA_PORT (3232)
typedef struct
{
    size_t size;
    char *data;
} log_info_t;

static int nlog_udp_send(const char *buffer, size_t size);


static QueueHandle_t network_log_queue = NULL;
#define NETWORK_LOG_QUEUE_NUM   (60)
#define NETWORK_LOG_QUEUE_SIZE  (sizeof(log_info_t))

static volatile esp_network_log_level_t _set_level;
static volatile bool _initialized;
static WiFiUDP _udp_log;

static void nlog_send_task(void *pvParameters)
{
    log_info_t log_info;
    while (1)
    {
        if (xQueueReceive(network_log_queue, &log_info, pdMS_TO_TICKS(MDEBUG_LOG_TIMEOUT_MS)) != pdPASS) 
        {
            continue;
        }
        nlog_udp_send(log_info.data,log_info.size);
        ESP_LOG_REDIRECTION_FREE(log_info.data);
    }
    vTaskDelete(NULL);
}

static void nlog_receive_task(void *pvParameters)
{
    while (1)
    {
        if(_udp_log.parsePacket()>0)
        {
            if(_udp_log.peek() == '~')
            {
                _udp_log.flush();
            }
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void nlog_ota_task(void *pvParameters)
{
    while (1)
    {
        ArduinoOTA.handle();
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void nlog_deinit(void)
{
    if (_initialized==false)
    {
        log_w("already initialized");
        return;
    }
    log_i("network log stopped.");
    _initialized = false;
    _udp_log.stop();
    // delete task
}

void nlog_init(const char* ssid,const char* password,esp_network_log_level_t log_level,bool ota_enable)
{
    if (_initialized)
    {
        log_w("already initialized");
        return;
    }
    _set_level = log_level;
    uint8_t wifi_try_num = 3;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED ) 
    {
        Serial.println("Connection Failed! Rebooting...");
        delay(3000);
        if(wifi_try_num<= 0)
        {
            return;
        }
        wifi_try_num --;
    }
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if(ota_enable)
    {
        ArduinoOTA.setPort(TCP_OTA_PORT);
        ArduinoOTA
        .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        //   Serial.println("Start updating " + type);
        })
        .onEnd([]() {
        //   Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
        //   Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
        //   Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });

        ArduinoOTA.begin();
        xTaskCreateUniversal(nlog_ota_task, "nlog_ota_task",60*1024, NULL, 7, NULL, ARDUINO_RUNNING_CORE);
    }


    char hostname[20];
    uint8_t mac[6];
    WiFi.macAddress(mac);
    sprintf(hostname, "esp-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    MDNS.begin(hostname);
    MDNS.enableArduino(TCP_OTA_PORT, false);
    
    _initialized = true;
    if(!_udp_log.begin(UDP_LOG_PORT))
    {
        log_e("udp bind failed");
        return;
    }

    network_log_queue= xQueueCreate(NETWORK_LOG_QUEUE_NUM, NETWORK_LOG_QUEUE_SIZE);
	if( network_log_queue == 0 )
    {
        printf("failed to create queue1\r\n");
    }
    xTaskCreateUniversal(nlog_send_task, "nlog_send_task",8*1024, NULL, 6, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreateUniversal(nlog_receive_task, "nlog_receive_task",6*1024, NULL, 5, NULL, ARDUINO_RUNNING_CORE);
}


static int nlog_udp_send(const char *buffer, size_t size)
{
    int out;
    out = _udp_log.beginPacket(_udp_log.remoteIP(), _udp_log.remotePort());
    _udp_log.write((uint8_t*)buffer,size);
    out = _udp_log.endPacket();
    return out;
}

int nlog_available(void)
{
    if(_initialized)
    {
        return _udp_log.available();
    }
    return 0;
}

int nlog_read(char* buffer, size_t len)
{
    if(_initialized)
    {
        return _udp_log.read(buffer,len);
    }
    return 0;
}

static ssize_t nlog_vprintf(const char *fmt, va_list vp)
{
    size_t log_size = 0;
    log_info_t log_info;
    log_info.size =log_size = vasprintf(&log_info.data, fmt, vp);
    if (log_info.size == 0 )
    {
        ESP_LOG_REDIRECTION_FREE(log_info.data);
        return 0;
    }
    
    if (!network_log_queue || xQueueSend(network_log_queue, &log_info, 0) == pdFALSE) 
    {
        ESP_LOG_REDIRECTION_FREE(log_info.data);
    }
    return log_size;
}

void netlog_write(esp_network_log_level_t level, const char *tag, const char *format, ...)
{
    if((_set_level < level) || (_set_level == ESP_NLOG_NONE))
    {
        return;
    }
    va_list list;
    va_start(list, format);
    nlog_vprintf(format,list);
    va_end(list);
}
void nlog_flush(void)
{
    _udp_log.flush();
}

esp_network_log_t esp_network_log={
    nlog_init,
    nlog_available,
    nlog_read,
    nlog_flush,
    nlog_deinit,
};
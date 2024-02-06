#ifndef __ESP_NETWORK_LOG_H__
#define __ESP_NETWORK_LOG_H__
#include "esp_log.h"
#include "stdint.h"
#include "stddef.h"

#ifndef NLOG_LOCAL_LEVEL
#define NLOG_LOCAL_LEVEL 3
#endif

typedef enum {
    ESP_NLOG_NONE = 0,       /*!< No log output */
    ESP_NLOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    ESP_NLOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    ESP_NLOG_INFO,       /*!< Information messages which describe normal flow of events */
} esp_network_log_level_t;

typedef struct
{
    void (*init)(const char* ssid,const char* password,esp_network_log_level_t log_level,bool ota_enable);
    int (*available)(void);
    int (*read)(char* buffer, size_t len);
    void (*flush)(void);
    void (*deinit)(void);
}esp_network_log_t;
void netlog_write(esp_network_log_level_t level, const char *tag, const char *format, ...);


#define NLOG_COLOR_RED     "31"
#define NLOG_COLOR_GREEN   "32"
#define NLOG_COLOR_BROWN   "33"
#define NLOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define NLOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define NLOG_RESET_COLOR   "\033[0m"
#define NLOG_COLOR_E       NLOG_COLOR(NLOG_COLOR_RED)
#define NLOG_COLOR_W       NLOG_COLOR(NLOG_COLOR_BROWN)
#define NLOG_COLOR_I       NLOG_COLOR(NLOG_COLOR_GREEN)

#define NLOG_FORMAT(letter, format)  NLOG_COLOR_ ## letter #letter " (%d) %s: " format NLOG_RESET_COLOR "\n"

#if (defined NLOG_LOCAL_LEVEL) && (NLOG_LOCAL_LEVEL != 0)
#define ESP_NLOGE( tag, format, ... )  if (NLOG_LOCAL_LEVEL >= ESP_NLOG_ERROR)   { netlog_write(ESP_NLOG_ERROR,   tag, NLOG_FORMAT(E, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }

#define ESP_NLOGW( tag, format, ... )  if (NLOG_LOCAL_LEVEL >= ESP_NLOG_WARN)    { netlog_write(ESP_NLOG_WARN,    tag, NLOG_FORMAT(W, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }

#define ESP_NLOGI( tag, format, ... )  if (NLOG_LOCAL_LEVEL >= ESP_NLOG_INFO)    { netlog_write(ESP_NLOG_INFO,    tag, NLOG_FORMAT(I, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }
#else
#define ESP_NLOGE( tag, format, ... )

#define ESP_NLOGW( tag, format, ... )

#define ESP_NLOGI( tag, format, ... )
#endif

extern  esp_network_log_t esp_network_log;

#endif
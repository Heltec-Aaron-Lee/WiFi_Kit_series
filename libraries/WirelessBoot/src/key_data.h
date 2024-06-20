#ifndef __KEY_DATA_H_
#define __KEY_DATA_H_
#include "stdint.h"

#define KEY_DATA_SECTOR_SIZE 0X1000 //Sector size 4096/4K
//Each sector can have up to 64 sectors in 4k and 256k space to store data.

// Add a sector size to the previous one each time.
#define KEY_HELTEC_LICENSE_ADDRESS  0x00000000
#define KEY_HELTEC_LICENSE_LEN      (KEY_DATA_SECTOR_SIZE)

#define KEY_WIFI_USER_CONF_ADDRESS  (KEY_HELTEC_LICENSE_ADDRESS+KEY_HELTEC_LICENSE_LEN)
#define KEY_WIFI_USER_CONF_LEN      (KEY_DATA_SECTOR_SIZE)

#define KEY_APP_INFO_ADDRESS        (KEY_WIFI_USER_CONF_ADDRESS+KEY_WIFI_USER_CONF_LEN)
#define KEY_APP_INFO_LEN            (KEY_DATA_SECTOR_SIZE)

#define KEY_SECOND_APP_INFO_ADDRESS (KEY_APP_INFO_ADDRESS + KEY_APP_INFO_LEN )
#define KEY_SECOND_APP_INFO_LEN     (KEY_DATA_SECTOR_SIZE)

#define KEY_FLASH_APP_INFO_ADDRESS  (KEY_SECOND_APP_INFO_ADDRESS + KEY_SECOND_APP_INFO_LEN )
#define KEY_FLASH_APP_INFO_LEN      (KEY_DATA_SECTOR_SIZE)

/**************************************************************************************/
#define CONF_VALID_VAL  (0x56)
typedef struct 
{
    uint8_t conf_valid;
    char ssid[64];
    char password[128];
}wifi_user_conf_t;

#define FIRMWARE_NAME_LEN (64)
#define FIRMWARE_SIZE_LEN (64)
#define FIRMWARE_UPLOAD_TIME_LEN (64)
typedef enum
{
    FIRMWARE_VALID    = 0x32,
    FIRMWARE_INVALID  = 0x64
}firmware_valid_flag_t;

typedef struct 
{
    uint32_t partition_size;
    char firmware_name[FIRMWARE_NAME_LEN];
    char firmware_size[FIRMWARE_SIZE_LEN];
    char firmware_upload_time[FIRMWARE_UPLOAD_TIME_LEN];
    firmware_valid_flag_t firmware_valid_flag; 
}firmware_info_t;
/**************************************************************************************/
// uint32_t license[4]={0x3D6699E0, 0xA2C6CB8B, 0x48C94611, 0xEC6F9A41};

void key_set_heltec_license(void * license,uint16_t license_len);
void key_get_heltec_license(void * license,uint16_t license_len);

void key_set_wifi_user_conf(wifi_user_conf_t * conf,uint8_t conf_len);
void key_get_wifi_user_conf(wifi_user_conf_t * conf,uint8_t conf_len);

void key_set_app_info(void * app_info,uint16_t app_info_len);
void key_get_app_info(void * app_info,uint16_t app_info_len);

void key_set_second_app_info(void * second_app_info,uint16_t second_app_info_len);
void key_get_second_app_info(void * second_app_info,uint16_t second_app_info_len);

void key_set_flash_app_info(void * flash_app_info,uint16_t flash_app_info_len);
void key_get_flash_app_info(void * flash_app_info,uint16_t flash_app_info_len);

#endif

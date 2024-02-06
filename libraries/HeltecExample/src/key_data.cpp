#include "key_data.h"
// #include "spi_flash_mmap.h"
#include "esp_partition.h"
#include "esp32s3/rom/rtc.h"
#include "esp32s3/rom/spi_flash.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define KEY_DATA_TAG "KEY_DATA"
/*
  *The size of a single write and read cannot exceed one sector.
*/
static esp_err_t key_data_flash_read(void * buffer, uint32_t offset, uint32_t length)
{
    esp_err_t err;
    if(buffer ==NULL || (length > KEY_DATA_SECTOR_SIZE))
    {
        ESP_LOGE(KEY_DATA_TAG, "ESP_ERR_INVALID_ARG");
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *key_data_partition = esp_partition_find_first((esp_partition_type_t)0x40,(esp_partition_subtype_t)0x00,"key_data");
    if(key_data_partition == NULL)
    {
        ESP_LOGE(KEY_DATA_TAG, "Flash partition not found.");
        return ESP_FAIL;
    }

    err = esp_partition_read(key_data_partition, offset, buffer,length);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "Flash read failed.");
        return err;
    }
    return err;
}

static esp_err_t key_data_flash_write(void * buffer, uint32_t offset, uint32_t length)
{
    esp_err_t err;
    if(buffer ==NULL || (length > KEY_DATA_SECTOR_SIZE))
    {
        ESP_LOGE(KEY_DATA_TAG, "ESP_ERR_INVALID_ARG");
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *key_data_partition = esp_partition_find_first((esp_partition_type_t)0x40,(esp_partition_subtype_t)0x00,"key_data");
    if(key_data_partition == NULL)
    {
        ESP_LOGE(KEY_DATA_TAG, "Flash partition not found.");
        return ESP_FAIL;
    }

    err = esp_partition_erase_range(key_data_partition, offset, KEY_DATA_SECTOR_SIZE);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "Flash erase failed.");
        return err;
    }
    
    err = esp_partition_write(key_data_partition, offset, buffer, length);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "Flash write failed.");
        return err;
    }
    return err;
}

void key_set_heltec_license(void * license,uint16_t license_len)
{
    esp_err_t err; 
    err = key_data_flash_write(license,KEY_HELTEC_LICENSE_ADDRESS,license_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_set_heltec_license failed.");
    }
}

void key_get_heltec_license(void * license,uint16_t license_len)
{
    esp_err_t err; 
    err = key_data_flash_read(license,KEY_HELTEC_LICENSE_ADDRESS,license_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_get_heltec_license failed.");
    }
}


void key_set_wifi_user_conf(wifi_user_conf_t * conf,uint8_t conf_len)
{
    esp_err_t err; 
    err = key_data_flash_write(conf,KEY_WIFI_USER_CONF_ADDRESS,conf_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_set_wifi_user_conf failed.");
    }
}

void key_get_wifi_user_conf(wifi_user_conf_t * conf,uint8_t conf_len)
{
    esp_err_t err; 
    err = key_data_flash_read(conf,KEY_WIFI_USER_CONF_ADDRESS,conf_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_get_wifi_user_conf failed.");
    }
}

void key_set_app_info(void * app_info,uint16_t app_info_len)
{
    esp_err_t err; 
    err = key_data_flash_write(app_info,KEY_APP_INFO_ADDRESS,app_info_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_set_app_info failed.");
    }
}

void key_get_app_info(void * app_info,uint16_t app_info_len)
{
    esp_err_t err; 
    err = key_data_flash_read(app_info,KEY_APP_INFO_ADDRESS,app_info_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_get_app_info failed.");
    }
}

void key_set_second_app_info(void * second_app_info,uint16_t second_app_info_len)
{
    esp_err_t err; 
    err = key_data_flash_write(second_app_info,KEY_SECOND_APP_INFO_ADDRESS,second_app_info_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_set_second_app_info failed.");
    }
}

void key_get_second_app_info(void * second_app_info,uint16_t second_app_info_len)
{
    esp_err_t err; 
    err = key_data_flash_read(second_app_info,KEY_SECOND_APP_INFO_ADDRESS,second_app_info_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_get_second_app_info failed.");
    }
}

void key_set_flash_app_info(void * flash_app_info,uint16_t flash_app_info_len)
{
    esp_err_t err; 
    err = key_data_flash_write(flash_app_info,KEY_FLASH_APP_INFO_ADDRESS,flash_app_info_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_set_flash_app_info failed.");
    }
}

void key_get_flash_app_info(void * flash_app_info,uint16_t flash_app_info_len)
{
    esp_err_t err; 
    err = key_data_flash_read(flash_app_info,KEY_FLASH_APP_INFO_ADDRESS,flash_app_info_len);
    if(err != ESP_OK)
    {
        ESP_LOGE(KEY_DATA_TAG, "key_get_flash_app_info failed.");
    }
}

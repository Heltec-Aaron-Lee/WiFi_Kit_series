#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define HELTEC_RCC6 true

#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t TX = 16;
static const uint8_t RX = 17;

static const uint8_t SDA = 2;
static const uint8_t SCL = 1;

static const uint8_t SS = 23;
static const uint8_t MOSI = 22;
static const uint8_t MISO = 20;
static const uint8_t SCK = 21;

static const uint8_t A0 = 0;
static const uint8_t A1 = 1;
static const uint8_t A2 = 2;
static const uint8_t A3 = 3;
static const uint8_t A4 = 4;
static const uint8_t A5 = 5;
static const uint8_t A6 = 6;

static const uint8_t USER_BUTTON = 9;
static const uint8_t ADC_BATTERY_PIN = 6;
#define BAT_VOLT_PIN ADC_BATTERY_PIN
static const uint8_t ADC_BATTERY_CTRL_PIN = 5;

static const uint8_t Vext = 11;
static const uint8_t VEXT_LDO_CTRL = 11;

static const uint8_t RST_LoRa = 8;
static const uint8_t BUSY_LoRa = 10;
static const uint8_t DIO0 = 19;

#endif /* Pins_Arduino_h */

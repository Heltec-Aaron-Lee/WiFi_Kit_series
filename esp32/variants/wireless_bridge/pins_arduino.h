#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define Wireless_Bridge true

#define EXTERNAL_NUM_INTERRUPTS 16
#define NUM_DIGITAL_PINS        40
#define NUM_ANALOG_INPUTS       16

#define analogInputToDigitalPin(p)  (((p)<20)?(esp32_adc2gpio[(p)]):-1)
#define digitalPinToInterrupt(p)    (((p)<40)?(p):-1)
#define digitalPinHasPWM(p)         (p < 34)

static const uint8_t LED_BUILTIN = 25;
#define BUILTIN_LED  LED_BUILTIN // backward compatibility

static const uint8_t KEY_BUILTIN = 0;

static const uint8_t SDA = 21;
static const uint8_t SCL = 22;

static const uint8_t SS    = 18;
static const uint8_t MOSI  = 27;
static const uint8_t MISO  = 19;
static const uint8_t SCK   = 5;

static const uint8_t Vext = 21;
static const uint8_t LED  = 25;
static const uint8_t BLE_LED  = 25;
static const uint8_t WIFI_LED  = 23;
static const uint8_t LoRa_LED  = 22;
static const uint8_t RST_LoRa = 14;
static const uint8_t DIO0 = 26;
static const uint8_t DIO1 = 35;
static const uint8_t DIO2 = 34;

#endif /* Pins_Arduino_h */
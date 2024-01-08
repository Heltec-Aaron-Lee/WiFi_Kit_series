#ifndef LoRaWan_APP_H
#define LoRaWan_APP_H

#include "ESP32_Mcu.h"
#include <stdio.h>
#include "loramac/LoRaMac.h"
#include "loramac/utilities.h"
#include "ESP32_LoRaWan_102.h"
#include "HardwareSerial.h"
#include "Arduino.h"
#include "driver/board.h"
#include "driver/debug.h"

#if defined(__asr650x__)
#include "board.h"
#include "gpio.h"
#include "hw.h"
#include "low_power.h"
#include "spi-board.h"
#include "rtc-board.h"
#include "asr_timer.h"
#include "board-config.h"
#include "hw_conf.h"
#include <uart_port.h>
#endif

enum eDeviceState_LoraWan
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_SLEEP
};

enum eDeviceState_Lora
{
    LORA_INIT,
    LORA_SEND,
    LORA_RECEIVE,
    LORA_CAD,
    MCU_SLEEP,
};


extern uint8_t devEui[];
extern uint8_t appEui[];
extern uint8_t appKey[];
extern uint8_t nwkSKey[];
extern uint8_t appSKey[];
extern uint32_t devAddr;
extern uint8_t appData[LORAWAN_APP_DATA_MAX_SIZE];
extern uint8_t appDataSize;
extern uint8_t appPort;
extern uint32_t txDutyCycleTime;
extern bool overTheAirActivation;
extern LoRaMacRegion_t loraWanRegion;
extern bool loraWanAdr;
extern bool isTxConfirmed;
extern uint32_t appTxDutyCycle;
extern DeviceClass_t loraWanClass;
extern bool passthroughMode;
extern uint8_t confirmedNbTrials;
extern bool modeLoraWan;
extern bool keepNet;
extern uint16_t userChannelsMask[6];

/*!
 * Defines a random delay for application data transmission duty cycle. 1s,
 * value in [ms].
 */
#define APP_TX_DUTYCYCLE_RND                        1000

class LoRaWanClass{
public:
  void init(DeviceClass_t lorawanClass,LoRaMacRegion_t region);
  void join();
  void send();
  void cycle(uint32_t dutyCycle);
  void sleep(DeviceClass_t classMode);
  void setDataRateForNoADR(int8_t dataRate);
  void ifskipjoin();
  void generateDeveuiByChipID();

#if defined(WIFI_LoRa_32_V3)||defined(Wireless_Track)||defined(WIFI_LoRa_32_V2)||defined(Wireless_Stick_V3)
  void displayJoining();
  void displayJoined();
  void displaySending();
  void displayAck();
  void displayMcuInit();
#endif
};

extern enum eDeviceState_LoraWan deviceState;

extern "C" bool SendFrame( void );
extern "C" void turnOnRGB(uint32_t color,uint32_t time);
extern "C" void turnOffRGB(void);
extern "C" bool checkUserAt(char * cmd, char * content);
extern "C" void downLinkAckHandle();
extern "C" void downLinkDataHandle(McpsIndication_t *mcpsIndication);
extern "C" void lwan_dev_params_update( void );
extern "C" void dev_time_updated( void );


extern LoRaWanClass LoRaWAN;

#endif

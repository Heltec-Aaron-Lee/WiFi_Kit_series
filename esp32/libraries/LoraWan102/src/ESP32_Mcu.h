#ifndef McuSet_H
#define McuSet_H

#include <Arduino.h>
#include "SPI.h"
#include "driver/rtc-board.h"
#include "driver/board-config.h"
#include "driver/lorawan_spi.h"


#define LORA_DEFAULT_NSS_PIN    18
#define LORA_DEFAULT_RESET_PIN  14
#define LORA_DEFAULT_DIO0_PIN   26
#define LORA_DEFAULT_DIO1_PIN   33
#define Timer_DEFAULT_DIV       80
extern uint8_t mcuStarted;
class McuClass{
public:
  McuClass();
  void setlicense(uint32_t * license);
  int begin();
  void setSPIFrequency(uint32_t frequency);
  void sleep(uint8_t classMode,uint8_t debugLevel);
  SPISettings _spiSettings;
private:

};
extern TimerEvent_t TxNextPacketTimer;

#ifdef __cplusplus
extern "C" uint8_t SpiInOut(Spi_t *obj, uint8_t outData );
extern "C" void lora_printf(const char *format, ...);
extern "C" uint64_t timercheck();
extern "C" uint64_t getID();
extern "C" void SX126xIoInit( void );
extern "C" void SX126xReset( void );
extern "C" void sx126xSleep( void );
#endif

extern McuClass Mcu;
#endif

/*
/ _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
\____ \| ___ |    (_   _) ___ |/ ___)  _ \
  _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
(C)2013 Semtech

Description: Target board general functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "../driver/board.h"
#include "Arduino.h"


/*
* MCU objects
*/

#if defined( USE_USB_CDC )
Uart_t UartUsb;
#endif

/*!
* Initializes the unused GPIO to a know status
*/
static void BoardUnusedIoInit( void );

/*!
* System Clock Configuration
*/
static void SystemClockConfig( void );

/*!
* Used to measure and calibrate the system wake-up time from STOP mode
*/
static void CalibrateSystemWakeupTime( void );

/*!
* System Clock Re-Configuration when waking up from STOP mode
*/
static void SystemClockReConfig( void );

/*!
* Timer used at first boot to calibrate the SystemWakeupTime
*/
static TimerEvent_t CalibrateSystemWakeupTimeTimer;

/*!
* Flag to indicate if the MCU is Initialized
*/
static bool McuInitialized = false;

/*!
* Flag to indicate if the SystemWakeupTime is Calibrated
*/
static bool SystemWakeupTimeCalibrated = false;

/*!
* Callback indicating the end of the system wake-up time calibration
*/
static void OnCalibrateSystemWakeupTimeTimerEvent( void )
{
  SystemWakeupTimeCalibrated = true;
}

/*!
* Nested interrupt counter.
*
* \remark Interrupt should only be fully disabled once the value is 0
*/
static uint8_t IrqNestLevel = 0;

void BoardDisableIrq( void )
{
    noInterrupts();
    IrqNestLevel++;
}

void BoardEnableIrq( void )
{
  IrqNestLevel--;
  if( IrqNestLevel == 0 )
  {
	  interrupts();
  }
}

void BoardInitPeriph( void )
{  

}

#define FIFO_TX_SIZE      512
uint8_t UARTTxBuffer[FIFO_TX_SIZE];

#define FIFO_RX_SIZE      512
uint8_t UARTRxBuffer[FIFO_RX_SIZE];


void BoardInitMcu( void )
{
}

void BoardDeInitMcu( void )
{
}


/*!
* Factory power supply
*/
#define FACTORY_POWER_SUPPLY                        3300 // mV

/*!
* VREF calibration value
*/
#define VREFINT_CAL                                 ( *( uint16_t* )0x1FF80078 )

/*!
* ADC maximum value
*/
#define ADC_MAX_VALUE                               4095

/*!
* Battery thresholds
*/
#define BATTERY_MAX_LEVEL                           4150 // mV
#define BATTERY_MIN_LEVEL                           3200 // mV
#define BATTERY_SHUTDOWN_LEVEL                      3100 // mV

static uint16_t BatteryVoltage = BATTERY_MAX_LEVEL;

uint16_t BoardBatteryMeasureVolage( void )
{
  //    uint16_t vdd = 0;
  //    uint16_t vref = VREFINT_CAL;
  //    uint16_t vdiv = 0;
  uint16_t batteryVoltage = 0;
  
  //    vdiv = AdcReadChannel( &Adc, BAT_LEVEL_CHANNEL );
  //    //vref = AdcReadChannel( &Adc, ADC_CHANNEL_VREFINT );
  //
  //    vdd = ( float )FACTORY_POWER_SUPPLY * ( float )VREFINT_CAL / ( float )vref;
  //    batteryVoltage = vdd * ( ( float )vdiv / ( float )ADC_MAX_VALUE );
  //
  //    //                                vDiv
  //    // Divider bridge  VBAT <-> 470k -<--|-->- 470k <-> GND => vBat = 2 * vDiv
  //    batteryVoltage = 2 * batteryVoltage;
  return batteryVoltage;
}

uint32_t BoardGetBatteryVoltage( void )
{
  return BatteryVoltage;
}

uint8_t BoardGetBatteryLevel( void )
{
  return 0;
}

static void BoardUnusedIoInit( void )
{

}

void SystemClockConfig( void )
{
  
}

void CalibrateSystemWakeupTime( void )
{
  if( SystemWakeupTimeCalibrated == false )
  {
    TimerInit( &CalibrateSystemWakeupTimeTimer, OnCalibrateSystemWakeupTimeTimerEvent );
    TimerSetValue( &CalibrateSystemWakeupTimeTimer, 1000 );
    TimerStart( &CalibrateSystemWakeupTimeTimer );
    while( SystemWakeupTimeCalibrated == false )
    {
      TimerLowPowerHandler( );
    }
  }
}

void SystemClockReConfig( void )
{

}

void SysTick_Handler( void )
{

}

uint8_t GetBoardPowerSource( void )
{
#if defined( USE_USB_CDC )
  if( GpioRead( &UsbDetect ) == 1 )
  {
    return BATTERY_POWER;
  }
  else
  {
    return USB_POWER;
  }
#else
  return BATTERY_POWER;
#endif
}

uint32_t HexToString(/*IN*/  const char    * pHex,  
                     /*IN*/  uint32_t           hexLen,  
                     /*OUT*/ char          * pByteString)  
{  
  unsigned long i;  
  
  if (pHex==NULL)  
    return 1;  
  
  if(hexLen <= 0)  
    return 2;  
  
  for(i=0;i<hexLen;i++)  
  {  
    if(((pHex[i]&0xf0)>>4)>=0 && ((pHex[i]&0xf0)>>4)<=9)  
      pByteString[2*i]=((pHex[i]&0xf0)>>4)+0x30;  
    else if(((pHex[i]&0xf0)>>4)>=10 && ((pHex[i]&0xf0)>>4)<=16)  
      pByteString[2*i]=((pHex[i]&0xf0)>>4)+0x37;   //  小写：0x37 改为 0x57   
    
    if((pHex[i]&0x0f)>=0 && (pHex[i]&0x0f)<=9)  
      pByteString[2*i+1]=(pHex[i]&0x0f)+0x30;  
    else if((pHex[i]&0x0f)>=10 && (pHex[i]&0x0f)<=16)  
      pByteString[2*i+1]=(pHex[i]&0x0f)+0x37;      //  小写：0x37 改为 0x57   
  }  
  return 0;  
} 

#ifdef USE_FULL_ASSERT
/*
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*/
void assert_failed( uint8_t* file, uint32_t line )
{
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  /* Infinite loop */
  while( 1 )
  {
  }
}
#endif

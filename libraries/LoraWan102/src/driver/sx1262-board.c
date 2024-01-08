/*!
 * \file      sx1262dvk1cas-board.c
 *
 * \brief     Target board SX1262DVK1CAS shield driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */

#if defined(WIFI_LoRa_32_V3)||defined(Wireless_Track)||defined(Wireless_Mini_Shell)||defined(WIFI_LoRa_32_SX1262)||defined(Wireless_Stick_V3)
#include <stdlib.h>
#include "../driver/sx126x-board.h"
#include "../driver/sx126x.h"
#include "Arduino.h"
#include "../driver/board-config.h"

uint8_t gPaOptSetting = 0;

extern uint8_t SpiInOut(Spi_t *obj, uint8_t outData );
SX126x_t SX126x;
uint32_t SX126xGetBoardTcxoWakeupTime( void )
{
    return BOARD_TCXO_WAKEUP_TIME;
}

void SX126xIoInit( void )
{
    GpioInit( &SX126x.Reset, RADIO_RESET, OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
    GpioInit( &SX126x.Spi.Nss, RADIO_NSS, OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
    GpioInit( &SX126x.BUSY, RADIO_BUSY, INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX126x.DIO1, RADIO_DIO_1, INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

void SX126xIoIrqInit( DioIrqHandler dioIrq )
{
//	attachInterrupt(RADIO_DIO_1,dioIrq,RISING);
    GpioSetInterrupt( &SX126x.DIO1, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY, dioIrq );
}

void SX126xReset( void )
{
	pinMode(SX126x.Reset.pin,OUTPUT);
	delay( 20 );
	digitalWrite(SX126x.Reset.pin,1);
	delay( 50 );
	digitalWrite(SX126x.Reset.pin,0);
	delay( 50 );
	digitalWrite(SX126x.Reset.pin,1);
	delay( 50 );

    /*
    GpioInit( &SX126x.Reset, RADIO_RESET, OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );
    delay( 40 );
    GpioInit( &SX126x.Reset, RADIO_RESET, OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 ); // internal pull-up
    delay( 40 );
    GpioInit( &SX126x.Reset, RADIO_RESET, OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );
    */
}

void SX126xWaitOnBusy( void )
{
	uint32_t timeout=0;
    while( GpioRead( &SX126x.BUSY ) == 1 )
    {
    	timeout++;
    	if(timeout>1000000)
    	{
    		lora_printf("spi timeout\r\n");
    		break;
    	}
    	delayMicroseconds(1);
    	//lora_printf("b");
    }
}

void SX126xWakeup( void )
{
    GpioWrite( &SX126x.Spi.Nss, 0 );
    SpiInOut( &SX126x.Spi, RADIO_GET_STATUS );
    SpiInOut( &SX126x.Spi, 0x00 );
    GpioWrite( &SX126x.Spi.Nss, 1 );;
    // Wait for chip to be ready.
    SX126xWaitOnBusy( );
}


void SX126xWriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );
    GpioWrite( &SX126x.Spi.Nss, 0 );
    SpiInOut( &SX126x.Spi, ( uint8_t )command );

    for( uint16_t i = 0; i < size; i++ )
    {
        SpiInOut( &SX126x.Spi, buffer[i] );
    }

    GpioWrite( &SX126x.Spi.Nss, 1 );

    if( command != RADIO_SET_SLEEP )
    {
        SX126xWaitOnBusy( );
    }
}

void SX126xReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{

    SX126xCheckDeviceReady( );
    GpioWrite( &SX126x.Spi.Nss, 0 );

    SpiInOut( &SX126x.Spi, ( uint8_t )command );
    SpiInOut( &SX126x.Spi, 0x00 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( &SX126x.Spi, 0 );
    }

    GpioWrite( &SX126x.Spi.Nss, 1 );

    SX126xWaitOnBusy( );
}

void SX126xWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );

    GpioWrite( &SX126x.Spi.Nss, 0 );

    SpiInOut( &SX126x.Spi, RADIO_WRITE_REGISTER );
    SpiInOut( &SX126x.Spi, ( address & 0xFF00 ) >> 8 );
    SpiInOut( &SX126x.Spi, address & 0x00FF );
    
    for( uint16_t i = 0; i < size; i++ )
    {
        SpiInOut( &SX126x.Spi, buffer[i] );
    }

    GpioWrite( &SX126x.Spi.Nss, 1 );

    SX126xWaitOnBusy( );
}

void SX126xWriteRegister( uint16_t address, uint8_t value )
{
    SX126xWriteRegisters( address, &value, 1 );
}

void SX126xReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );

    GpioWrite( &SX126x.Spi.Nss, 0 );

    SpiInOut( &SX126x.Spi, RADIO_READ_REGISTER );
    SpiInOut( &SX126x.Spi, ( address & 0xFF00 ) >> 8 );
    SpiInOut( &SX126x.Spi, address & 0x00FF );
    SpiInOut( &SX126x.Spi, 0 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( &SX126x.Spi, 0 );
    }
    GpioWrite( &SX126x.Spi.Nss, 1 );

    SX126xWaitOnBusy( );
}

uint8_t SX126xReadRegister( uint16_t address )
{
    uint8_t data;
    SX126xReadRegisters( address, &data, 1 );
    return data;
}

void SX126xWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{

    SX126xCheckDeviceReady( );

    GpioWrite( &SX126x.Spi.Nss, 0 );

    SpiInOut( &SX126x.Spi, RADIO_WRITE_BUFFER );
    SpiInOut( &SX126x.Spi, offset );
    for( uint16_t i = 0; i < size; i++ )
    {
        SpiInOut( &SX126x.Spi, buffer[i] );
    }
    GpioWrite( &SX126x.Spi.Nss, 1 );

    SX126xWaitOnBusy( );
}

void SX126xReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    SX126xCheckDeviceReady( );

    GpioWrite( &SX126x.Spi.Nss, 0 );

    SpiInOut( &SX126x.Spi, RADIO_READ_BUFFER );
    SpiInOut( &SX126x.Spi, offset );
    SpiInOut( &SX126x.Spi, 0 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( &SX126x.Spi, 0 );
    }
    GpioWrite( &SX126x.Spi.Nss, 1 );

    SX126xWaitOnBusy( );

}

void SX126xSetRfTxPower( int8_t power )
{
    SX126xSetTxParams( power, RADIO_RAMP_3400_US );
}

uint8_t SX126xGetPaSelect( uint32_t channel )
{
    return SX1262;
}

void SX126xAntSwOn( void )
{
}

void SX126xAntSwOff( void )
{
}

bool SX126xCheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

uint8_t SX126xGetPaOpt( )
{
    return gPaOptSetting;
}

void SX126xSetPaOpt( uint8_t opt )
{
    if(opt>3) return;
    
    gPaOptSetting = opt;
}

#endif

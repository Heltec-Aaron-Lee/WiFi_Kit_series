/*!
 * \file      gpio-board.c
 *
 * \brief     Target board GPIO driver implementation
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
#include "../loramac/utilities.h"
#include "../driver/rtc-board.h"
#include "../driver/gpio-board.h"


static GpioIrqHandler *GpioIrq[16];

void GpioMcuInit( Gpio_t *obj, uint8_t pin, uint8_t mode, PinConfigs config, PinTypes type, uint32_t value )
{
	obj->pin = pin;
	pinMode(pin,mode);
    if( mode == OUTPUT )
    {
        GpioMcuWrite( obj, value );
    }
}

void GpioMcuSetInterrupt( Gpio_t *obj, int irqMode, IrqPriorities irqPriority, GpioIrqHandler *irqHandler )
{
	attachInterrupt(obj->pin,irqHandler,RISING);
}

void GpioMcuRemoveInterrupt( Gpio_t *obj )
{
	detachInterrupt(obj->pin);
}

void GpioMcuWrite( Gpio_t *obj, uint32_t value )
{
	digitalWrite(obj->pin,value);
}

void GpioMcuToggle( Gpio_t *obj )
{

}

uint32_t GpioMcuRead( Gpio_t *obj )
{
	return digitalRead(obj->pin);
}

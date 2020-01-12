
// taken from core_esp8266_wiring.c in esp8266/arduino
// https://github.com/esp8266/Arduino/blob/master/cores/esp8266/core_esp8266_wiring.c

/* 
 core_esp8266_wiring.c - implementation of Wiring API for esp8266

 Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef __cplusplus
extern "C"
{
#endif
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "esp-millis.h"
#ifdef __cplusplus
}
#endif

static os_timer_t micros_overflow_timer;
static uint32_t micros_at_last_overflow_tick = 0;
static uint32_t micros_overflow_count = 0;

#define ONCE 0
#define REPEAT 1

void micros_overflow_tick(void* arg) __attribute__((weak));
void micros_overflow_tick(void* arg) {
    (void) arg;
    uint32_t m = system_get_time();
    if(m < micros_at_last_overflow_tick)
        ++micros_overflow_count;
    micros_at_last_overflow_tick = m;
}

uint32_t millis(void) __attribute__((weak));
uint32_t millis(void) {
    static int once = 0;
    if (!once) {
        os_timer_setfn(&micros_overflow_timer, (os_timer_func_t*) &micros_overflow_tick, 0);
        os_timer_arm(&micros_overflow_timer, 60000, REPEAT);
        once = 1;
    }

    uint32_t m = system_get_time();
    uint32_t c = micros_overflow_count + ((m < micros_at_last_overflow_tick) ? 1 : 0);
    return c * 4294967 + m / 1000;
}


// parts of https://github.com/esp8266/Arduino/blob/master/cores/esp8266/time.c

/*
 * time.c - ESP8266-specific functions for SNTP
 * Copyright (c) 2015 Peter Dobler. All rights reserved.
 * This file is part of the esp8266 core for Arduino environment.
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <time.h>
#ifdef __cplusplus
extern "C"
{
#endif
#include "sntp.h"
#include "esp-millis.h"
#ifdef __cplusplus
}
#endif


#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED
struct timeval {
  time_t      tv_sec;
  suseconds_t tv_usec;
};
#endif

uint32_t micros (void);

static time_t s_bootTime = 0;

// calculate offset used in gettimeofday
static void ensureBootTimeIsSet()
{
    if (!s_bootTime)
    {
        time_t now = sntp_get_current_timestamp();
        if (now)
            s_bootTime =  now - millis() / 1000;
    }
}

int gettimeofday(struct timeval *tp, void *tzp)
{
    (void) tzp;
    if (tp)
    {
        ensureBootTimeIsSet();
        tp->tv_sec  = s_bootTime + millis() / 1000;
        tp->tv_usec = micros() * 1000;
    }
    return 0;
}

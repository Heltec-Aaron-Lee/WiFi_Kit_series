
# lwIP-2 from original source

This repo offers a link layer for esp82xx-nonos-sdk-2.
The original goal is to try and use a recent lwIP version for stability reasons.
Currently lwIP-v2 is implemented, other IP stacks could be tried.

current lwIP link: [STABLE_2.1.2](http://git.savannah.nongnu.org/cgit/lwip.git/tree/?h=STABLE-2_1_2_RELEASE)

# Status

* UDP/TCP codes using lwIP-1.4 need some updates - two examples: [arduino](https://github.com/esp8266/Arduino/pull/3129) and [sming](https://github.com/SmingHub/Sming/pull/1147)
* IPv6 working in Arduino
* SACK-out enabled in Arduino
* tcp is more stable ([example1](https://github.com/esp8266/Arduino/issues/3075) and [example2](https://github.com/esp8266/Arduino/issues/2925))
* used by default in esp8266 arduino core
* accepted pull request in [esp8266/arduino](https://github.com/esp8266/Arduino/pull/3783)
* (WIP) [sming](https://github.com/SmingHub/Sming/pull/1147)
* (WIP) [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk/pull/271)

# Build

[with Arduino](https://github.com/esp8266/Arduino/tree/master/tools/sdk/lwip2)

# MSS

Remember the MSS footprint: 4\*MSS bytes in RAM per tcp connection.
The lowest recommanded value is 536.

# How it works

Espressif binary libraries rely on their lwip implementation. The idea, as
described in this [comment](https://github.com/kadamski/esp-lwip/issues/8)
is to wrap espressif calls, and rewrite them for a new tcp implementation.

Example with lwip1.4's ethernet_input() called by espressif binary blobs
finally reaching lwip2's:

```
-- LWIP2-----------------------------------
#define ethernet_input ethernet_input_LWIP2
- lwip2's ethernet_input_LWIP2_is called
                            (/ \)
                             | |
-- glue (new side)-----------^-v-----------
                             | |
glue_ethernet_input          | |
- maps structures glue->new  |
- calls ethernet_input_LWIP2(^ v)
- maps structures new->glue    |
                             | |
-- glue (old side)-----------^-v-----------
                             | |
ethernet_input():            | |
- maps structures old->glue  | 
- calls glue_ethernet_input (^ v)
- maps structures glue->old    |
                             | |
- espressif blobs -----------^-v-----------
XXXXXXXXXXXXXXXXXXXXXXXXXXXX | | XXXXXXXXXX
wifi calls    ethernet_input(/ \) XXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
-------------------------------------------
```

# History

Original [cleaning and port](https://github.com/nekromant/esp8266-frankenstein/tree/master/src/contrib/lwipupdate) of espressif's patch set from lwIP-v1.4 to lwIP-v2 with references to lwIP-git-sha1

[Discussion](https://github.com/kadamski/esp-lwip/issues/8) on how further work could done

[First](https://github.com/d-a-v/esp8266-phy) version of this implementation

[Second](https://github.com/esp8266/Arduino/pull/3206) version for arduino only

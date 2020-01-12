#!/bin/bash

# this is needed for esp-open-sdk
# this script changes '#include "lwip/xxx"' into '#include <lwip/xxx>'

{
echo "patch-non-local-includes"

! [[ -f "$1/sntp.h" ]] && echo "$1/sntp.h does not exit." && exit 1;
! [[ -f "$1/user_interface.h" ]] && echo "$1/user_interface.h does not exit." && exit 1;

#################################### sntp.h ####################################
if ! [[ -f "$1/sntp.h.orig" ]]; then
	cp "$1/sntp.h" "$1/sntp.h.orig";
	sed -i -e 's|ip_addr_t|ipv4_addr_t|g' "$1/sntp.h"
	sed -i -e 's|#ifdef LWIP_OPEN_SRC|\n#include <lwip/init.h>\n#include <lwip/ip_addr.h>\n|g' "$1/sntp.h"
	sed -i -e 's|#include "lwip/ip_addr.h"|#if LWIP_VERSION_MAJOR == 1\n#define ipv4_addr_t ip_addr_t|g' "$1/sntp.h"
	sed -i -e 's|#include "ip_addr.h"|typedef struct ip4_addr ipv4_addr_t;|g' "$1/sntp.h"
fi
################################################################################

############################### user_interface.h ###############################
# cp -f .backups/user_interface.h user_interface.h
if ! [[ -f "$1/user_interface.h.orig" ]]; then
	cp "$1/user_interface.h" "$1/user_interface.h.orig";
	sed -i -e 's|struct ip_addr|struct ipv4_addr|g' "$1/user_interface.h"
	sed -i -e 's|#include "lwip/ip_addr.h"|#include <lwip/init.h>\n\n#if LWIP_VERSION_MAJOR == 1\n#define ipv4_addr ip_addr\n#endif\n#include <lwip/ip_addr.h>\n#if LWIP_VERSION_MAJOR != 1\ntypedef struct ip4_addr ipv4_addr_t;\n#endif\n|g' "$1/user_interface.h";
	sed -i -e 's|#include "ip_addr.h"|#error LWIP_OPEN_SRC must be defined|g' "$1/user_interface.h"
fi
################################################################################

################################## ipv_addr.h ##################################
if ! [[ -f "$1/ip_addr.h.orig" ]]; then
cp "$1/ip_addr.h" "$1/ip_addr.h.orig";

cat <<"EOF" > "$1/ip_addr.h"
#ifndef __IP_ADDR_H__
#define __IP_ADDR_H__

#include "c_types.h"
#include "ipv4_addr.h"

/**
 * Determine if two address are on the same network.
 *
 * @arg addr1 IP address 1
 * @arg addr2 IP address 2
 * @arg mask network identifier mask
 * @return !0 if the network identifiers of both address match
 */
#define ipv4_addr_netcmp(addr1, addr2, mask) (((addr1)->addr & \
        (mask)->addr) == \
        ((addr2)->addr & \
         (mask)->addr))

/** Set an IP address given by the four byte-parts.
    Little-endian version that prevents the use of htonl. */
#define IP4_ADDR(ipaddr, a,b,c,d) \
        (ipaddr)->addr = ((uint32)((d) & 0xff) << 24) | \
                         ((uint32)((c) & 0xff) << 16) | \
                         ((uint32)((b) & 0xff) << 8)  | \
                          (uint32)((a) & 0xff)

#define ipv4_addr1(ipaddr) (((uint8*)(ipaddr))[0])
#define ipv4_addr2(ipaddr) (((uint8*)(ipaddr))[1])
#define ipv4_addr3(ipaddr) (((uint8*)(ipaddr))[2])
#define ipv4_addr4(ipaddr) (((uint8*)(ipaddr))[3])

#define ipv4_addr1_16(ipaddr) ((uint16)ipv4_addr1(ipaddr))
#define ipv4_addr2_16(ipaddr) ((uint16)ipv4_addr2(ipaddr))
#define ipv4_addr3_16(ipaddr) ((uint16)ipv4_addr3(ipaddr))
#define ipv4_addr4_16(ipaddr) ((uint16)ipv4_addr4(ipaddr))


/** 255.255.255.255 */
#define IPADDR_NONE         ((uint32)0xffffffffUL)
/** 0.0.0.0 */
#define IPADDR_ANY          ((uint32)0x00000000UL)
uint32 ipaddr_addr(const char *cp);

#define IP2STR(ipaddr) ipv4_addr1_16(ipaddr), \
    ipv4_addr2_16(ipaddr), \
    ipv4_addr3_16(ipaddr), \
    ipv4_addr4_16(ipaddr)

#define IPSTR "%d.%d.%d.%d"

#endif /* __IP_ADDR_H__ */
EOF
fi
################################################################################

################################# ipv4_addr.h ##################################
cat <<"EOF" > "$1/ipv4_addr.h"
#ifndef __IPV4_ADDR_H__
#define __IPV4_ADDR_H__

#include <stdint.h>
#include <lwip/init.h>

// ipv4_addr is necessary for lwIP-v2 because
// - espressif binary firmware is IPv4 only, under the name of ip_addr/_t
// - ip_addr/_t is different when IPv6 is enabled with lwIP-v2
// hence ipv4_addr/t is IPv4 version/copy of IPv4 ip_addr/_t
// when IPv6 is enabled so we can deal with IPv4 use from firmware API.

// official lwIP's definitions (1.4 or 2)
//#include "lwip/ip_addr.h"
#include <lwip/ip_addr.h>

///////////////////////////////////////////////
#if LWIP_VERSION_MAJOR == 1

#define ipv4_addr ip_addr

///////////////////////////////////////////////
#else // lwIP-v2

#define ipv4_addr ip4_addr
#define ipv4_addr_t ip4_addr_t

// defined in lwip-v1.4 sources only, used in fw
struct ip_info {
    struct ipv4_addr ip;
    struct ipv4_addr netmask;
    struct ipv4_addr gw;
};

///////////////////////////////////////////////
#endif // lwIP-v2

#endif // __IPV4_ADDR_H__
EOF
################################################################################

echo "patch-non-local-includes done"

} 1>&2

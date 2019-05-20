#!/bin/bash

# this is needed for esp-open-sdk
# this script adds a couple missing definitions to the clean vendor lwipopts.h
#
# +#define PBUF_RSV_FOR_WLAN
# +#define EBUF_LWIP
#

{
echo "patch-vendor-lwip14"

string_in_file() { cat "$1"|grep -q "$2" && echo "Found $2" && return 0; return 1; }
sr_file() { sed -i -e "s|$1|$2|g" $3; } # s|searchfor|replacewith|g thefile

! [[ -f "$1/lwipopts.h" ]] && echo "$1/lwipopts.h does not exit." && exit 1;
! [[ -f "$1/lwipopts.h.orig" ]] && cp "$1/lwipopts.h" "$1/lwipopts.h.orig";
! [[ -f "$1/lwip/mem.h.orig" ]] && cp "$1/lwip/mem.h" "$1/lwip/mem.h.orig";

echo "looking for PBUF_RSV_FOR_WLAN + EBUF_LWIP"

string_in_file "$1/lwipopts.h" '#define PBUF_RSV_FOR_WLAN'; found1=$?;
string_in_file "$1/lwipopts.h" '#define EBUF_LWIP'; found2=$?;

if [[ $found1 -eq 0 ]] && [[ $found2 -eq 0 ]]; then
	echo "Skipping lwipopts.h patch";
else
	sed -i -e 's|#define __LWIPOPTS_H__|#define __LWIPOPTS_H__\n\n#define PBUF_RSV_FOR_WLAN\n#define EBUF_LWIP|g' "$1/lwipopts.h"
fi

echo "Patching vendor mem.h definitions"

sed -i -e 's|#define mem_free vPortFree|#define mem_free(p) vPortFree(p, "", 0)|g' "$1/lwip/mem.h"
sed -i -e 's|#define mem_malloc pvPortMalloc|#define mem_malloc(s) pvPortMalloc(s, "", 0)|g' "$1/lwip/mem.h"
sed -i -e 's|#define mem_calloc pvPortCalloc|#define mem_calloc(l, s) pvPortCalloc(l, s, "", 0)|g' "$1/lwip/mem.h"
sed -i -e 's|#define mem_realloc pvPortRealloc|#define mem_realloc(p, s) pvPortRealloc(p, s, "", 0)|g' "$1/lwip/mem.h"
sed -i -e 's|#define mem_zalloc pvPortZalloc|#define mem_zalloc(s) pvPortZalloc(s, "", 0)|g' "$1/lwip/mem.h"

} 1>&2

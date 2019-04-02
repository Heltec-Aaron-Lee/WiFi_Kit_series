
#ifndef LWIP_GIT_H
#define LWIP_GIT_H

#include "lwipopts.h"
#include "lwip/netif.h"
#include "user_interface.h"

#define netif_sta (&netif_git[STATION_IF])
#define netif_ap  (&netif_git[SOFTAP_IF])
extern struct netif netif_git[2];

#endif // LWIP_GIT_H

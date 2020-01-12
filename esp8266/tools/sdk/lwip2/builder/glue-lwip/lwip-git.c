
/*

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution. 
3. The name of the author may not be used to endorse or promote products 
derived from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.

author: d. gauchard

*/

// lwip2(git) side of glue

#include "glue.h"
#include "uprint.h"
#include "lwip-helper.h"

#include "lwipopts.h"
#include "lwip/err.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "lwip/apps/sntp.h"
#include "arch/cc.h"

#if LWIP_IPV6
#include "lwip/ethip6.h"
#include "lwip/dhcp6.h"
#endif

#include "lwip-git.h"

// this is dhcpserver taken from lwip-1.4-espressif
#include "lwip/apps-esp/dhcpserver.h"
// this is espconn taken from lwip-1.4-espressif
#include "lwip/apps-esp/espconn.h"

#define DBG "GLUE: "

//#define netif_sta (&netif_git[STATION_IF])
//#define netif_ap  (&netif_git[SOFTAP_IF])

struct netif netif_git[2];
const char netif_name[2][8] = { "station", "soft-ap" };

int __attribute__((weak)) doprint_allow = 0; // for doprint()

uint32_t SNTP_UPDATE_DELAY __attribute__((weak));
uint32_t SNTP_UPDATE_DELAY
{
    return SNTP_UPDATE_DELAY_DEFAULT;
}

uint32_t SNTP_STARTUP_DELAY_FUNC __attribute__((weak));
uint32_t SNTP_STARTUP_DELAY_FUNC
{
    return SNTP_STARTUP_DELAY_FUNC_DEFAULT;
}

err_t glue2git_err (err_glue_t err)
{
	switch (err)
	{
	case GLUE_ERR_OK         : return ERR_OK;
	case GLUE_ERR_MEM        : return ERR_MEM;
	case GLUE_ERR_BUF        : return ERR_BUF;
	case GLUE_ERR_TIMEOUT    : return ERR_TIMEOUT;
	case GLUE_ERR_RTE        : return ERR_RTE;
	case GLUE_ERR_INPROGRESS : return ERR_INPROGRESS;
	case GLUE_ERR_VAL        : return ERR_VAL;
	case GLUE_ERR_WOULDBLOCK : return ERR_WOULDBLOCK;
	case GLUE_ERR_USE        : return ERR_USE;
	case GLUE_ERR_ALREADY    : return ERR_ALREADY;
	case GLUE_ERR_ISCONN     : return ERR_ISCONN;
	case GLUE_ERR_CONN       : return ERR_CONN;
	case GLUE_ERR_IF         : return ERR_IF;
	case GLUE_ERR_ABRT       : return ERR_ABRT;
	case GLUE_ERR_RST        : return ERR_RST;
	case GLUE_ERR_CLSD       : return ERR_CLSD;
	case GLUE_ERR_ARG        : return ERR_ARG;
	
	default: return ERR_ABRT;
	}
};	

err_glue_t git2glue_err (err_t err)
{
	switch (err)
	{
	case ERR_OK         : return GLUE_ERR_OK;
	case ERR_MEM        : return GLUE_ERR_MEM;
	case ERR_BUF        : return GLUE_ERR_BUF;
	case ERR_TIMEOUT    : return GLUE_ERR_TIMEOUT;
	case ERR_RTE        : return GLUE_ERR_RTE;
	case ERR_INPROGRESS : return GLUE_ERR_INPROGRESS;
	case ERR_VAL        : return GLUE_ERR_VAL;
	case ERR_WOULDBLOCK : return GLUE_ERR_WOULDBLOCK;
	case ERR_USE        : return GLUE_ERR_USE;
	case ERR_ALREADY    : return GLUE_ERR_ALREADY;
	case ERR_ISCONN     : return GLUE_ERR_ISCONN;
	case ERR_CONN       : return GLUE_ERR_CONN;
	case ERR_IF         : return GLUE_ERR_IF;
	case ERR_ABRT       : return GLUE_ERR_ABRT;
	case ERR_RST        : return GLUE_ERR_RST;
	case ERR_CLSD       : return GLUE_ERR_CLSD;
	case ERR_ARG        : return GLUE_ERR_ARG;

	default: return GLUE_ERR_ABRT;
	}
};	

#if UDEBUG

static void new_display_netif_flags (int flags)
{
	#define IFF(x)	do { if (flags & NETIF_FLAG_##x) uprint("|" #x); } while (0)
	IFF(UP);
	IFF(BROADCAST);
	IFF(LINK_UP);
	IFF(ETHARP);
	IFF(ETHERNET);
	IFF(IGMP);
	IFF(MLD6);
	#undef IFF
}

static void new_display_netif (struct netif* netif)
{
	uprint(DBG "lwip-@%p idx=%d(%s) mtu=%d state=%p ",
		netif,
		netif->num,
		netif_name[netif->num],
		netif->mtu,
		netif->state);
	if (netif->hwaddr_len == 6)
		display_mac(netif->hwaddr);
	new_display_netif_flags(netif->flags);
	display_ip32(" ip=", ip_2_ip4(&netif->ip_addr)->addr);
	display_ip32(" mask=", ip_2_ip4(&netif->netmask)->addr);
	display_ip32(" gw=", ip_2_ip4(&netif->gw)->addr);
	uprint("\n");
}

#else // !UDEBUG

#define new_display_netif_flags(x) do { (void)0; } while (0)
#define new_display_netif(x) do { (void)0; } while (0)

#endif // !UDEBUG

int lwiperr_check (const char* what, err_t err)
{
	if (err != ERR_OK)
	{
		uerror("ERROR: %s (error %d)\n", what, (int)err);
		return 0;
	}
	return 1;
}

err_glue_t esp2glue_dhcp_start (int netif_idx)
{
	uprint(DBG "dhcp_start netif: ");
	new_display_netif(&netif_git[netif_idx]);

	// set link and interface up for dhcp client
	netif_set_link_up(&netif_git[netif_idx]);
	// calls netif_sta_status_callback() - if applicable (STA)
	netif_set_up(&netif_git[netif_idx]);

	// Update to latest esp hostname before starting dhcp client,
	// 	because this name is provided to the dhcp server.
	// Until proven wrong, dhcp client is the only code
	// 	needing netif->hostname.
	// Then obviously user application needs to set hostname
	// 	before starting wifi station if dhcp is used.
	// XXX to check: is wifi_station_get_hostname()
	// 	returning a const pointer once _set_hostname is called?
	netif_git[netif_idx].hostname = wifi_station_get_hostname();

	err_t err = dhcp_start(&netif_git[netif_idx]);
#if LWIP_IPV6 && LWIP_IPV6_DHCP6_STATELESS
	if (err == ERR_OK)
		err = dhcp6_enable_stateless(&netif_git[netif_idx]);
#endif
	uprint(DBG "new_dhcp_start returns %d\n", (int)err);
	return git2glue_err(err);
}

void esp2glue_dhcp_stop (int netif_idx)
{
	uprint(DBG "dhcp_stop\n");
	dhcp_stop(&netif_git[netif_idx]);
}

// a pbuf flag bit for our own use
#define PBUF_FLAG_GLUED 0x40U   // check unicity in pbuf.h:"#define PBUF_FLAG_*"

err_t new_linkoutput (struct netif* netif, struct pbuf* p)
{
	#if !LWIP_NETIF_TX_SINGLE_PBUF
	#warning ESP netif->linkoutput cannot handle pbuf chains.
	#warning LWIP_NETIF_TX_SINGLE_PBUF should be 1 in lwipopts.h (but now works without)
	#endif

	if (p->next || (p->flags & PBUF_FLAG_GLUED))
	{
		//printf("COPIED\n");
	    // PBUF_FLAG_GLUED: this packet has been allocated by us below (esp2glue_alloc_for_recv())
	    // and needs to be duplicated before sending to esp side (otherwise content is destroyed,
	    // see comment below). This happens when a received packet is forwarded (ex: by napt).

		// Otherwise, !!p->next should not happen since LWIP_NETIF_TX_SINGLE_PBUF=1
		// it can however happen:
		// see https://git.savannah.gnu.org/cgit/lwip.git/tree/src/include/lwip/opt.h#n1593
		// see http://lists.nongnu.org/archive/html/lwip-users/2017-10/msg00059.html

		// make a monolithic pbuf from a fragmented one by copying it
		struct pbuf* q = pbuf_clone(PBUF_LINK, PBUF_RAM, p);
		if (q == NULL)
			return ERR_MEM;
		uprint("UNCHAIN: p(ref=%d len=%d tot=%d)->q(ref=%d len=%d tot=%d pl=%p nxt=%p)\n", p->ref, p->len, p->tot_len, q->ref, q->len, q->tot_len, q->payload, q->next);
		p = q;
		// old p will be released by caller
		// new p = q will be released by glue2esp_linkoutput() subsequent callbacks to esp2glue_pbuf_freed()
	}
	else
	{
		// protect pbuf, so lwip2(git) won't free it before phy(esp) finishes sending
		pbuf_ref(p);
		//printf("NOT COPIED\n");
    }

	uassert(netif->num == STATION_IF || netif->num == SOFTAP_IF);

	uprint(DBG "linkoutput: netif@%p (%s) pbuf=%p payload=%p\n", netif, netif_name[netif->num], p, p->payload);
	uprint(DBG "linkoutput default netif: %d\n", netif_default? netif_default->num: -1);

	// when packet is forwarded by NAPT but not cloned above, p->payload *value* is changed after this call:
	err_t err = glue2git_err(glue2esp_linkoutput(netif->num, p, p->payload, p->len));

	if (err != ERR_OK)
	{
		uprint(DBG "linkoutput error sending pbuf@%p\n", p);
		pbuf_free(p); // release pbuf_ref() above (or free pbuf_clone()->q->p)
	}

	return err;
}

void esp2glue_pbuf_freed (void* pbuf)
{
	uprint(DBG "blobs release lwip-pbuf (ref=%d) @%p\n", ((struct pbuf*)pbuf)->ref, pbuf);
	pbuf_free((struct pbuf*)pbuf);
}

static err_t new_input (struct pbuf *p, struct netif *inp)
{
	(void)p;
	(void)inp;
	//uerror("internal error, new-netif->input() cannot be called\n");
	uassert(0);
	return ERR_ABRT;
}

void esp2glue_netif_set_default (int netif_idx)
{
	if (netif_idx == STATION_IF || netif_idx == SOFTAP_IF)
	{
		uprint(DBG "netif_set_default %s\n", netif_name[netif_idx]);
		netif_set_default(&netif_git[netif_idx]);
	}
	else
	{
		uprint(DBG "netif_set_default NULL\n");
		netif_set_default(NULL);
	}
}

static void netif_sta_status_callback (struct netif* netif)
{
	// address can be set or reset/any (=0)

	uprint(DBG "netif status callback:\n");
	new_display_netif(netif);
	
	// tell ESP that link is updated
	glue2esp_ifupdown(netif->num, ip_2_ip4(&netif->ip_addr)->addr, ip_2_ip4(&netif->netmask)->addr, ip_2_ip4(&netif->gw)->addr);

	if (   netif->flags & NETIF_FLAG_UP
	    && netif == netif_sta)
	{
		// this is our default route
		netif_set_default(netif);
			
		// If we have a valid address of any type restart SNTP
		bool valid_address = ip_2_ip4(&netif->ip_addr)->addr;

#if LWIP_IPV6
		int addrindex;
		for (addrindex = 0; addrindex < LWIP_IPV6_NUM_ADDRESSES; addrindex++) {
			valid_address |= ip6_addr_isvalid(netif_ip6_addr_state(netif, addrindex));
		}
#endif
		if (valid_address)		{
			// restart sntp
			sntp_stop();
			sntp_init();
		}
	}
}

static void netif_init_common (struct netif* netif)
{
	netif->flags |= NETIF_FLAG_IGMP;
	// irrelevant,not used since esp-lwip receive data and call esp2glue_ethernet_input()
	netif->input = new_input;
	// meaningfull:
	netif->output = etharp_output;
	netif->linkoutput = new_linkoutput;

#if LWIP_IPV6
	netif->output_ip6 = ethip6_output;
	netif->ip6_autoconfig_enabled = 1;
#endif
	
	netif->hostname = wifi_station_get_hostname();
	netif->chksum_flags = NETIF_CHECKSUM_ENABLE_ALL;
	// netif->mtu given by glue
}

static err_t netif_init_sta (struct netif* netif)
{
	uprint(DBG "netif_sta_init\n");
	
	netif->name[0] = 's';
	netif->name[1] = 't';
	netif->status_callback = netif_sta_status_callback; // need to tell esp-netif-sta is up
	
	netif_init_common(netif);
	
	return ERR_OK;
}

static err_t netif_init_ap (struct netif* netif)
{
	uprint(DBG "netif_ap_init\n");

	netif->name[0] = 'a';
	netif->name[1] = 'p';
	netif->status_callback = NULL; // esp-netif-ap is made up by esp
	
	netif_init_common(netif);

	return ERR_OK;
}

void esp2glue_netif_update (int netif_idx, uint32_t ip, uint32_t mask, uint32_t gw, size_t hwlen, const uint8_t* hwaddr, uint16_t mtu)
{
	uprint(DBG "netif updated:\n");

	struct netif* netif = &netif_git[netif_idx];

	if (hwlen && hwlen <= sizeof(netif->hwaddr))
	{
		netif->hwaddr_len = hwlen;
		memcpy(netif->hwaddr, hwaddr, netif->hwaddr_len = hwlen);
	}
	
	// properly set netif state according to IP address
	// (we could pass netif flags though)
	// so we don't later tell esp that we are up with a bad(=0) IP address
	// (fixes sdk-2.2.1 disconnection/reconnection bug)
	// TODO: now interface management is working quite better than in old
	// XXX   times on esp-side, take time and properly propagate netif's
	// XXX   (UP, LINK_UP) states into here
	netif->flags |= NETIF_FLAG_LINK_UP;
	if (ip)
		netif->flags |= NETIF_FLAG_UP;
	else
		netif->flags &= ~NETIF_FLAG_UP;

	netif->mtu = mtu;
	netif->hostname = wifi_station_get_hostname();
	ip4_addr_t aip = { ip }, amask = { mask }, agw = { gw };
	netif_set_addr(&netif_git[netif_idx], &aip, &amask, &agw);
	esp2glue_netif_set_up1down0(netif_idx, 1);

	netif_git[netif_idx].flags |= NETIF_FLAG_ETHARP;
	netif_git[netif_idx].flags |= NETIF_FLAG_BROADCAST;

#if LWIP_IPV6
	netif_git[netif_idx].flags |= NETIF_FLAG_MLD6;
	netif_create_ip6_linklocal_address(&netif_git[netif_idx], 1/*from mac*/);
#endif
	new_display_netif(&netif_git[netif_idx]);
}

void esp2glue_lwip_init (void)
{
	uprint(DBG "lwip_init\n");
	lwip_init();
	
	// we *must* add new interfaces ourselves so interface index are correct
	// esp may call netif_add() with idx=1 before idx=0
	ip4_addr_t aip = { 0 }, amask = { 0 }, agw = { 0 };
	for (int i = 0; i < 2; i++)
	{
		netif_add(&netif_git[i], &aip, &amask, &agw, /*state*/NULL,
		          i == STATION_IF? netif_init_sta: netif_init_ap,
		          /*useless input*/NULL);
		netif_git[i].hwaddr_len = NETIF_MAX_HWADDR_LEN;
		memset(netif_git[i].hwaddr, 0, NETIF_MAX_HWADDR_LEN);
	}
	
	sntp_servermode_dhcp(1); /* get SNTP server via DHCP */
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	// start anyway the offline sntp timer
#if ARDUINO
	SNTP_SET_SYSTEM_TIME_US(0,0);
#else
	sntp_set_system_time(0);
#endif
}

void esp2glue_espconn_init(void)
{
	espconn_init();
}

void esp2glue_alloc_for_recv (size_t len, void** pbuf, void** data)
{
	*pbuf = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
	if (*pbuf)
	{
		*data = ((struct pbuf*)*pbuf)->payload;
        ((struct pbuf*)*pbuf)->flags |= PBUF_FLAG_GLUED;
    }
}

err_glue_t esp2glue_ethernet_input (int netif_idx, void* received)
{
	// this input is allocated by esp2glue_alloc_for_recv()

	//uprint(DBG "input idx=%d netif-flags=0x%x ", netif_idx, netif_git[netif_idx].flags);
	//display_ip32(" ip=", netif_git[netif_idx].ip_addr.addr);
	//nl();
	
	return git2glue_err(ethernet_input((struct pbuf*)received, &netif_git[netif_idx]));
}

void esp2glue_dhcps_start (struct ip_info* info)
{
	dhcps_start(info);
}

void esp2glue_netif_set_up1down0 (int netif_idx, int up1_or_down0)
{
	uprint(DBG "netif %d: set %s\n", netif_idx, up1_or_down0? "up": "down");
	struct netif* netif = &netif_git[netif_idx];
	if (up1_or_down0)
	{
		netif_set_link_up(netif);
		//netif_set_up(netif); // unwanted call to netif_sta_status_callback()
		netif->flags |= NETIF_FLAG_UP;
	}
	else
	{
		// need to do this and pass it to esp
		// (through netif_sta_status_callback())
		// to update users's view of state
		memset(&netif->ip_addr, 0, sizeof(netif->ip_addr));
		memset(&netif->netmask, 0, sizeof(netif->netmask));
		memset(&netif->gw, 0, sizeof(netif->gw));

		netif_set_link_down(netif);
		netif_set_down(netif);

		if (netif_default == &netif_git[netif_idx])
			netif_set_default(NULL);
	}
}

#define VALUE_TO_STRING(x) #x
#define VAR_NAME_VALUE(var) "-------- " #var " = "  VALUE_TO_STRING(var) " --------\n"
#pragma message "\n\n" VAR_NAME_VALUE(TCP_MSS) VAR_NAME_VALUE(LWIP_FEATURES) VAR_NAME_VALUE(LWIP_IPV6)

LWIP_ERR_T lwip_unhandled_packet (struct pbuf* pbuf, struct netif* netif)
{
	// must pbuf_free(pbuf) if packet recognized and managed then return ERR_OK
	// default: not recognized
	// this function may be redefined by user
	(void)pbuf;
	(void)netif;
	return ERR_ARG;
}

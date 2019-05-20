
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

// esp(lwip1.4) side of glue for esp8266
// - sdk-2.0.0(656edbf)
// - sdk-2.1.0(116b762)
// - sdk-2.2.1(cfd48f3)

#include "glue.h"
#include "uprint.h"

#include "arch/cc.h"
#include "lwip/timers.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "netif/etharp.h"
#include "lwip/mem.h"

#define DBG	"lwESP: "
#define STUB(x) do { uerror("STUB: " #x "\n"); } while (0)

// guessed interface, esp blobs
void system_pp_recycle_rx_pkt (void*);
void system_station_got_ip_set(ip_addr_t* ip, ip_addr_t* mask, ip_addr_t* gw);

// ethbroadcast linked from blobs
const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};
// linked from blobs
struct netif *netif_default;

///////////////////////////////////////
// from pbuf.c
#define SIZEOF_STRUCT_PBUF	LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf))
// from pbuf.h
#ifndef PBUF_RSV_FOR_WLAN
#error PBUF_RSV_FOR_WLAN should be defined
#endif
#ifndef EBUF_LWIP
#error EBUF_LWIP should be defined
#endif
#define EP_OFFSET 36

///////////////////////////////////////
// netif

#define netif_sta netif_esp[STATION_IF]
#define netif_ap  netif_esp[SOFTAP_IF]
static struct netif* netif_esp[2] = { NULL, NULL };

///////////////////////////////////////
// glue converters

err_t glue2esp_err (err_glue_t err)
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
	case GLUE_ERR_ABRT       : return ERR_ABRT;
	case GLUE_ERR_RST        : return ERR_RST;
	case GLUE_ERR_CLSD       : return ERR_CLSD;
	case GLUE_ERR_CONN       : return ERR_CONN;
	case GLUE_ERR_ARG        : return ERR_ARG;
	case GLUE_ERR_USE        : return ERR_USE;
	case GLUE_ERR_IF         : return ERR_IF;
	case GLUE_ERR_ISCONN     : return ERR_ISCONN;

	/* old does not have: */
	case GLUE_ERR_ALREADY    : return ERR_ABRT;

	default: return ERR_ABRT;
	}
};

err_glue_t esp2glue_err (err_t err)
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
	case ERR_ABRT       : return GLUE_ERR_ABRT;
	case ERR_RST        : return GLUE_ERR_RST;
	case ERR_CLSD       : return GLUE_ERR_CLSD;
	case ERR_CONN       : return GLUE_ERR_CONN;
	case ERR_ARG        : return GLUE_ERR_ARG;
	case ERR_USE        : return GLUE_ERR_USE;
	case ERR_IF         : return GLUE_ERR_IF;
	case ERR_ISCONN     : return GLUE_ERR_ISCONN;

	default: return GLUE_ERR_ABRT;
	}
};

///////////////////////////////////////
// display helpers

#if UDEBUG

#define stub_display_ip(pre,ip) display_ip32(pre, (ip).addr)

static void stub_display_netif_flags (int flags)
{
	#define IFF(x)	do { if (flags & NETIF_FLAG_##x) uprint("|" #x); } while (0)
	IFF(UP);
	IFF(BROADCAST);
	IFF(POINTTOPOINT);
	IFF(DHCP);
	IFF(LINK_UP);
	IFF(ETHARP);
	IFF(ETHERNET);
	IFF(IGMP);
	#undef IFF
}

static void stub_display_netif (struct netif* netif)
{
	uprint(DBG "esp-@%p idx=%d %s name=%c%c%d state=%p ",
		netif, netif->num,
		netif->num == SOFTAP_IF? "AP": netif->num == STATION_IF? "STA": "???",
		netif->name[0], netif->name[1], netif->num,
		netif->state);
	if (netif->hwaddr_len == 6)
		display_mac(netif->hwaddr);
	else
		uprint("(no mac?)");
	stub_display_netif_flags(netif->flags);
	display_ip32(" ip=", netif->ip_addr.addr);
	display_ip32(" mask=", netif->netmask.addr);
	display_ip32(" gw=", netif->gw.addr);
	uprint("\n");
}

void pbuf_info (const char* what, pbuf_layer layer, u16_t length, pbuf_type type)
{
	uerror(DBG "%s layer=%s(%d) len=%d type=%s(%d)\n",
		what,
		layer==PBUF_TRANSPORT? "transport":
		layer==PBUF_IP? "ip":
		layer==PBUF_LINK? "link":
		layer==PBUF_RAW? "raw":
		"???", (int)layer,
		length,
		type==PBUF_RAM? "ram":
		type==PBUF_ROM? "rom":
		type==PBUF_REF? "ref":
		type==PBUF_POOL? "pool":
		type==PBUF_ESF_RX? "esp-wlan":
		"???", (int)type);
}

#else // !UDEBUG

#define stub_display_netif_flags(x) do { (void)0; } while (0)
#define stub_display_netif(x) do { (void)0; } while (0)
#define pbuf_info(x,y,z,w) do { (void)0; } while (0)

#endif // !UDEBUG

///////////////////////////////////////
// quick pool to store references to data sent

#define PBUF_CUSTOM_TYPE_POOLED 0x42 // must not conflict with PBUF_* (pbuf types)
#define PBUF_WRAPPER_BLOCK 8

struct pbuf_wrapper
{
	struct pbuf pbuf;		// must be first in pbuf_wrapper
	void* ref2save;			// pointer to keep aside this pbuf
	struct pbuf_wrapper* next;	// chain of unused
};

static struct pbuf_wrapper* pbuf_wrapper_head = NULL;	// first free

static struct pbuf_wrapper* pbuf_wrapper_get (void)
{
	ets_intr_lock();

	if (!pbuf_wrapper_head)
	{
		struct pbuf_wrapper* p = (struct pbuf_wrapper*)os_malloc(sizeof(struct pbuf_wrapper) * PBUF_WRAPPER_BLOCK);
		if (!p)
		{
			ets_intr_unlock();
			return NULL;
		}
		for (int i = 0; i < PBUF_WRAPPER_BLOCK; i++)
		{
			p->pbuf.type = PBUF_CUSTOM_TYPE_POOLED;	// constant
			p->pbuf.flags = 0;			// constant
			p->pbuf.next = NULL;			// constant
			p->pbuf.eb = NULL;			// constant
			p->next = i? p - 1: NULL;
			p++;
		}
		pbuf_wrapper_head = p - 1;
	}
	struct pbuf_wrapper* ret = pbuf_wrapper_head;
	pbuf_wrapper_head = pbuf_wrapper_head->next;

	ets_intr_unlock();

	return ret;
}

static void pbuf_wrapper_release (struct pbuf_wrapper* p)
{
	// make it the new head in the chain of unused
	ets_intr_lock();

	p->next = pbuf_wrapper_head;
	pbuf_wrapper_head = p;

	ets_intr_unlock();
}

err_glue_t glue2esp_linkoutput (int netif_idx, void* ref2save, void* data, size_t size)
{
	struct netif* netif = netif_esp[netif_idx];

	if (!netif)
	{
		uprint(DBG "linkoutput: netif %d not initialized\n", netif_idx);
		return GLUE_ERR_IF;
	}

	if (!(netif->flags & NETIF_FLAG_LINK_UP))
	{
		uprint(DBG "linkoutput(netif %d): link is not up\n", netif_idx);
		return GLUE_ERR_IF;
	}

	struct pbuf_wrapper* p = pbuf_wrapper_get();
	if (!p)
	{
		uprint(DBG "linkoutput(netif %d): memory full\n", netif_idx);
		return GLUE_ERR_MEM;
	}

	uassert(p->pbuf.type == PBUF_CUSTOM_TYPE_POOLED);
	uassert(p->pbuf.flags == 0);
	uassert(p->pbuf.next == NULL);
	uassert(p->pbuf.eb == NULL);
	
	p->pbuf.payload = data;
	p->pbuf.len = p->pbuf.tot_len = size;
	p->pbuf.ref = 0;
	p->ref2save = ref2save;

	uprint(DBG "linkoutput: real pbuf sent to wilderness (len=%dB esp-pbuf=%p glue-pbuf=%p payload=%p netif=%d)\n",
		p->pbuf.len,
		&p->pbuf,
		ref2save,
		data,
		netif_idx);
	
	// call blobs
	// blobs will call pbuf_free() back later
	// we will retrieve our ref2save and give it back to glue

	err_t err = netif->linkoutput(netif, &p->pbuf);

	if (phy_capture)
		phy_capture(netif->num, p->pbuf.payload, p->pbuf.len, /*out*/1, /*success*/err == ERR_OK);

	if (err != ERR_OK)
	{
		// blob/phy is exhausted, release memory
		pbuf_wrapper_release(p);
		uprint(DBG "wifi-output: error %d\n", (int)err);
	}
	return esp2glue_err(err);
}

///////////////////////////////////////
// STUBS / wrappers

void lwip_init (void)
{
	uprint(DBG "lwip_init\n");
	esp2glue_lwip_init();
}

err_t etharp_output (struct netif* netif, struct pbuf* q, ip_addr_t* ipaddr)
{
	(void)netif; (void)q; (void)ipaddr;
	//uerror("ERROR: STUB etharp_output should not be called\n");
	return ERR_ABRT;
}
                   
err_t ethernet_input (struct pbuf* p, struct netif* netif)
{
	uprint(DBG "received pbuf@%p (pbuf: %dB ref=%d eb=%p) on netif ", p, p->tot_len, p->ref, p->eb);
	stub_display_netif(netif);
	
	uassert(p->tot_len == p->len && p->ref == 1);
	
#if UDUMP
	// dump packets for me (direct or broadcast)
	if (   memcmp((const char*)p->payload, netif->hwaddr, 6) == 0
	    || memcmp((const char*)p->payload, ethbroadcast.addr, 6) == 0)
	{
		dump("ethinput", p->payload, p->len);
	}
#endif
	// copy esp pbuf to glue pbuf

	void* glue_pbuf;
	void* glue_data;

	// ask glue for space to store payload into
	esp2glue_alloc_for_recv(p->len, &glue_pbuf, &glue_data);

	if (phy_capture)
		phy_capture(netif->num, p->payload, p->len, /*out*/0, /*success*/!!glue_pbuf);

	if (glue_pbuf)
	{
		// copy data
		os_memcpy(glue_data, p->payload, p->len);
		uprint(DBG "copy esp-payload=%p -> glue-pbuf=%p payload=%p\n", p->payload, glue_pbuf, glue_data);
	}

	// release blob's buffer asap
	pbuf_free(p);

	if (!glue_pbuf)
		// packet lost
		return ERR_MEM;

	// pass to new ip stack
	uassert(netif->num == 0 || netif->num == 1);
	return glue2esp_err(esp2glue_ethernet_input(netif->num, glue_pbuf));
}

void dhcps_start (struct ip_info* info)
{
	uprint(DBG "dhcps_start ");
	display_ip_info(info);
	uprint("\n");
	
	if (netif_ap)
		///XXX this is mandatory for blobs to be happy
		// but we should get this info back through glue
	 	//netif_ap->flags |= NETIF_FLAG_UP | NETIF_FLAG_LINK_UP;
	 	esp2glue_netif_set_up1down0(netif_ap->num, 1);

 	esp2glue_dhcps_start(info);
}

void espconn_init (void)
{
	// called at boot/reset
	// annoying message to hide:
	//STUB(espconn_init);
#if OPENSDK
	esp2glue_espconn_init();
#endif
}

void dhcp_cleanup (struct netif* netif)
{
	// not implemented yet
	(void)netif;
	// message yet unseen
	STUB(dhcp_cleanup);
}

err_t dhcp_release (struct netif* netif)
{
	// not implemented yet
	(void)netif;
	// message yet unseen
	STUB(dhcp_release);
	return ERR_ABRT;
}

err_t dhcp_start (struct netif* netif)
{
	uprint(DBG "dhcp_start ");
	stub_display_netif(netif);

	// for lwip-v2: NETIF_FLAG_LINK_UP is mandatory for both input and output
	netif->flags |= NETIF_FLAG_LINK_UP;
	err_t err = glue2esp_err(esp2glue_dhcp_start(netif->num));
	return err;
}

void dhcp_stop (struct netif* netif)
{
	esp2glue_dhcp_stop(netif->num);
}

static int netif_is_new (struct netif* netif)
{
	struct netif* test_netif_sta = eagle_lwip_getif(STATION_IF);
	struct netif* test_netif_ap = eagle_lwip_getif(SOFTAP_IF);
	uprint(DBG "check netif @%p (sta@%p ap@%p)\n", netif, test_netif_sta, test_netif_ap);
	uassert(netif == test_netif_sta || netif == test_netif_ap);
	if (netif == test_netif_sta)
		netif->num = STATION_IF;
	else
		netif->num = SOFTAP_IF;

	netif->input = ethernet_input;

	if (netif_esp[netif->num] == netif)
	{
		uprint(DBG "netif (%d): already added\n", netif->num);
		return 0; // not new
	}
	
	uprint(DBG "NEW netif\n");
	stub_display_netif(netif);

	netif->next = netif->num == STATION_IF? test_netif_ap: NULL;
	netif_esp[netif->num] = netif;
	
	return 1; // is new
}

struct netif* netif_add (
	struct netif* netif,
	ip_addr_t* ipaddr,
	ip_addr_t* netmask,
	ip_addr_t* gw,
	void* state,
	netif_init_fn init,
	netif_input_fn packet_incoming)
{
	uprint(DBG "netif_add\n");
	
	//////////////////////////////
	// this is revisited ESP lwip implementation
	netif->ip_addr.addr = 0;
	netif->netmask.addr = 0;
	netif->gw.addr = 0;
	netif->flags = 0;

	#if LWIP_DHCP
	// ok
	netif->dhcp = NULL;
	netif->dhcps_pcb = NULL;
	#endif /* LWIP_DHCP */
		#if LWIP_AUTOIP
		#error
		netif->autoip = NULL;
		#endif /* LWIP_AUTOIP */
		#if LWIP_NETIF_STATUS_CALLBACK
		#error
		netif->status_callback = NULL;
		#endif /* LWIP_NETIF_STATUS_CALLBACK */
		#if LWIP_NETIF_LINK_CALLBACK
		#error
		netif->link_callback = NULL;
		#endif /* LWIP_NETIF_LINK_CALLBACK */
	#if LWIP_IGMP
	// ok
	netif->igmp_mac_filter = NULL;
	#endif /* LWIP_IGMP */
		#if ENABLE_LOOPBACK
		#error
		netif->loop_first = NULL;
		netif->loop_last = NULL;
		#endif /* ENABLE_LOOPBACK */
	netif->state = state;

	uassert(packet_incoming == ethernet_input);
	(void)packet_incoming;
	netif->input = ethernet_input;

		#if LWIP_NETIF_HWADDRHINT
		#error
		netif->addr_hint = NULL;
		#endif /* LWIP_NETIF_HWADDRHINT*/
		#if ENABLE_LOOPBACK && LWIP_LOOPBACK_MAX_PBUFS
		#error
		netif->loop_cnt_current = 0;
		#endif /* ENABLE_LOOPBACK && LWIP_LOOPBACK_MAX_PBUFS */

	// init() is from blobs to call blobs:
	// update: at least mtu is set by this function
	if (init(netif) != ERR_OK)
	{
		uprint(DBG "ERROR netif_add: caller's init() failed\n");
		return NULL;
	}

	uprint(DBG "netif_add(ip:%x) -> ", (int)ipaddr->addr);
	netif_set_addr(netif, ipaddr, netmask, gw);

	return netif;
}

void netif_remove (struct netif* netif)
{
	uprint(DBG "netif_remove -> netif_set_down");
	netif_set_down(netif);
}

static err_t voidinit (struct netif* netif)
{	
	(void)netif;
	return ERR_OK;
}

void netif_set_addr (struct netif* netif, ip_addr_t* ipaddr, ip_addr_t* netmask, ip_addr_t* gw)
{
	uprint(DBG "netif_set_addr(%x->%x)\n", (int)netif->ip_addr.addr, (int)ipaddr->addr);

	if (netif_is_new(netif))
	{
		// interface not yet properly added
		netif_add(netif, ipaddr, netmask, gw, netif->state, voidinit, ethernet_input);
		// netif_add() calls netif_set_addr()
		return;
	}

	netif->ip_addr.addr = ipaddr->addr;
	netif->netmask.addr = netmask->addr;
	netif->gw.addr = gw->addr;
	uassert(netif->input == ethernet_input);

	// ask blobs
	struct ip_info set;
	wifi_get_ip_info(netif->num, &set);
	if (!(set.ip.addr == ipaddr->addr && set.netmask.addr == netmask->addr && set.gw.addr == gw->addr))
	{
		// tell blobs
		set.ip.addr = ipaddr->addr;
		set.netmask.addr = netmask->addr;
		set.gw.addr = gw->addr;
		wifi_set_ip_info(netif->num, &set);
	}
	if (ipaddr->addr)
		netif->flags |= NETIF_FLAG_LINK_UP; // mandatory (enable reception)
	else
		netif->flags &= ~NETIF_FLAG_LINK_UP;

	stub_display_netif(netif);
	
	// esp2glue_netif_update calls
	//	=> lwip2 netif_set_addr()
	//	=> lwip2 netif->status_callback()
	//	=> lwip2 netif_sta_status_callback()
	//	=> esp   glue2esp_ifup()
	//	=> blobs's system_station_got_ip_set()
	esp2glue_netif_update(netif->num, ipaddr->addr, netmask->addr, gw->addr, netif->hwaddr_len, netif->hwaddr, netif->mtu);
}

void netif_set_default (struct netif* netif)
{
	uprint(DBG "netif_set_default %d\n", netif->num);
	netif_default = netif;
	
	// do not let sdk control lwip2's default interface
	// (softAP setting it to AP interface breaks routing,
	//  lwIP knows how to route)
	//no: esp2glue_netif_set_default(netif->num);
}

void netif_set_down (struct netif* netif)
{
	uprint(DBG "netif_set_down  ");
	stub_display_netif(netif);
	
	// this is an old comment and may be wrong
	// because since then netif handling has improved alot
	// (keeping it for now)
	//
	// dont set down. some esp8266 will:
	// * esp2glue_netif_set_down(sta)
	// * restart dhcp-client _without_ netif_set_up.
	// or:
	// * esp2glue_netif_set_down(ap)
	// * restart dhcp-server _without_ netif_set_up.

	netif->flags &= ~(NETIF_FLAG_UP | NETIF_FLAG_LINK_UP);
	esp2glue_netif_set_up1down0(netif->num, 0);

	// this seems sufficient
	//netif_disable(netif);
}

void netif_set_up (struct netif* netif)
{
	uprint(DBG "netif_set_up\n");
	stub_display_netif(netif);

	netif->flags |= (NETIF_FLAG_UP |  NETIF_FLAG_LINK_UP);
	esp2glue_netif_set_up1down0(netif->num, 1);
}

struct pbuf* pbuf_alloc (pbuf_layer layer, u16_t length, pbuf_type type)
{
	// pbuf creation from blobs
	// copy parts of original code matching specific requests

	//STUB(pbuf_alloc);
	//pbuf_info("pbuf_alloc", layer, length, type);
	
	u16_t offset = 0;
	if (layer == PBUF_RAW && type == PBUF_RAM)
	{
		offset += EP_OFFSET;
		
		/* If pbuf is to be allocated in RAM, allocate memory for it. */
		size_t alloclen = LWIP_MEM_ALIGN_SIZE(SIZEOF_STRUCT_PBUF + offset) + LWIP_MEM_ALIGN_SIZE(length);
		struct pbuf* p = (struct pbuf*)mem_malloc(alloclen);
		if (p == NULL)
			return NULL;
		/* Set up internal structure of the pbuf. */
		p->payload = LWIP_MEM_ALIGN((void *)((u8_t *)p + SIZEOF_STRUCT_PBUF + offset));
		p->len = p->tot_len = length;
		p->next = NULL;
		p->type = type;
		p->eb = NULL;
		p->ref = 1;
		p->flags = 0;
		uprint(DBG "pbuf_alloc(RAW/RAM)-> %p %dB type=%d\n", p, alloclen, type);
		return p;
	}
	
	if (layer == PBUF_RAW && type == PBUF_REF)
	{
		//unused: offset += EP_OFFSET;
		size_t alloclen = LWIP_MEM_ALIGN_SIZE(SIZEOF_STRUCT_PBUF);
		struct pbuf* p = (struct pbuf*)mem_malloc(alloclen);
		if (p == NULL)
			return NULL;
		p->payload = NULL;
		p->len = p->tot_len = length;
		p->next = NULL;
		p->type = type;
		p->eb = NULL;
		p->ref = 1;
		p->flags = 0;
		uprint(DBG "pbuf_alloc(RAW/REF)-> %p %dB type=%d\n", p, alloclen, type);
		return p;
	}

	uerror(DBG "pbuf_alloc BAD CASE\n");
		
	return NULL;
}

u8_t pbuf_free (struct pbuf *p)
{
	//STUB(pbuf_free);
	uprint(DBG "pbuf_free(%p) ref=%d type=%d\n", p, p->ref, p->type);
	//pbuf_info("pbuf_free", -1, p->len, p->type);
	//uprint("pbuf@%p ref=%d tot_len=%d eb=%p\n", p, p->ref, p->tot_len, p->eb);
	
	#if LWIP_SUPPORT_CUSTOM_PBUF
	#error LWIP_SUPPORT_CUSTOM_PBUF is defined
	#endif
	
	uassert(p->ref == 1);

	if (p->type == PBUF_CUSTOM_TYPE_POOLED)
	{
		// allocated by glue for sending packets
		uassert(!p->eb);
		// retrieve glue structure to be freed
		struct pbuf_wrapper* pw = (struct pbuf_wrapper*)p;
		// pw->ref2save is the glue structure to release
		uprint(DBG "pbuf_free chain release glue-pbuf %p lwip1-pbuf %p\n", pw->ref2save, (char*)p);
		uassert(pw->ref2save);
		esp2glue_pbuf_freed(pw->ref2save);
		pbuf_wrapper_release(pw);
			
		return 1;
	}
		
	if (   !p->next
	    && p->ref == 1
	    && (p->type == PBUF_RAM || p->type == PBUF_REF)
	   )
	{
		if (p->eb)
			system_pp_recycle_rx_pkt(p->eb);
		// allocated by blobs for received packets
		mem_free(p);
		return 1;
	}

	// never seen from the beginning (~1.5y), uerror => uprint, saves flash
	uprint("BAD CASE %p ref=%d tot_len=%d eb=%p\n", p, p->ref, p->tot_len, p->eb);
	return 0;
}

void pbuf_ref (struct pbuf *p)
{
	uprint(DBG "pbuf_ref(%p) ref=%d->%d\n", p, p->ref, p->ref + 1);
	++(p->ref);
}

void sys_timeout (u32_t msecs, sys_timeout_handler handler, void *arg)
{
	(void)msecs; (void)handler; (void)arg;
	// yet never seen
	STUB(sys_timeout);
}

void sys_untimeout (sys_timeout_handler handler, void *arg)
{
	(void)handler; (void)arg;
	// yet never seen
	STUB(sys_untimeout);
}

void glue2esp_ifupdown (int netif_idx, uint32_t ip, uint32_t mask, uint32_t gw)
{
	struct netif* netif = netif_esp[netif_idx];

	// backup old esp IPs
	ip_addr_t oldip, oldmask, oldgw;
	oldip = netif->ip_addr;
	oldmask = netif->netmask;
	oldgw = netif->gw;
	        
	uprint(DBG "glue2esp_ifupdown new %d.%d.%d.%d old %ld.%ld.%ld.%ld\n",
		        ip & 0xff,         (ip >> 8) & 0xff,         (ip >> 16) & 0xff,         ip >> 24,
		oldip.addr & 0xff, (oldip.addr >> 8) & 0xff, (oldip.addr >> 16) & 0xff, oldip.addr >> 24);

	// change IPs
	netif->ip_addr.addr = ip;
	netif->netmask.addr = mask;
	netif->gw.addr = gw;

	if (ip)
	{
		// set up
		netif->flags |= NETIF_FLAG_UP;
		// tell esp to check IP has changed (by giving old IPs)
		// only in case ip!=0
		system_station_got_ip_set(&oldip, &oldmask, &oldgw);
	}
	else
		// or down
		netif->flags &= ~NETIF_FLAG_UP;
}

void (*phy_capture) (int netif_idx, const char* data, size_t len, int out, int success) = NULL;

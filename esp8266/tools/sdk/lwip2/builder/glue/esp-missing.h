
#ifndef ESP_MISSING_H
#define ESP_MISSING_H

#include <stdarg.h>
#include <stdint.h>
#include <ipv4_addr.h>

// these declarations are missing from sdk and used by lwip1.4 from sdk2.0.0

uint32_t r_rand (void);

// TODO: Patch these in from SDK

#if ARDUINO
void* pvPortZalloc (size_t, const char*, int);
void* pvPortMalloc (size_t xWantedSize, const char* file, int line) __attribute__((malloc, alloc_size(1)));
void vPortFree (void *ptr, const char* file, int line);
#else
void *pvPortZalloc (size_t sz, const char *, unsigned);
void *pvPortMalloc (size_t sz, const char *, unsigned) __attribute__((malloc, alloc_size(1)));
void vPortFree (void *p, const char *, unsigned);
#endif

struct netif* eagle_lwip_getif (int netif_index);

void ets_intr_lock (void);
void ets_intr_unlock (void);

int ets_vprintf (int (*print_function)(int), const char * format, va_list arg) __attribute__ ((format (printf, 2, 0)));
int ets_sprintf (char *str, const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
int ets_putc(int);

void ets_bzero (void*, size_t);
int ets_memcmp (const void*, const void*, size_t n);
void *ets_memset (void *s, int c, size_t n);
void *ets_memcpy (void *dest, const void *src, size_t n);
void *ets_memmove(void *dest, const void *src, size_t n);

typedef void ETSTimerFunc(void *timer_arg);
void ets_timer_disarm (ETSTimer *a);
#if ARDUINO
void ets_timer_arm_new (ETSTimer *a, int b, int c, int isMstimer);
#else
void ets_timer_arm_new(os_timer_t *ptimer, uint32_t time, bool repeat_flag, bool ms_flag);
#endif
void ets_timer_setfn (ETSTimer *t, ETSTimerFunc *fn, void *parg);

struct ipv4_addr;
void wifi_softap_set_station_info (uint8_t* mac, struct ipv4_addr*);

#define os_intr_lock	ets_intr_lock
#define os_intr_unlock	ets_intr_unlock
#define os_bzero	ets_bzero
#define os_memcmp	ets_memcmp
#define os_memset	ets_memset
#define os_memcpy	ets_memcpy

#endif

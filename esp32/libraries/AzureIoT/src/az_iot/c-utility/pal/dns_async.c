// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// This file is OS-specific, and is identified by setting include directories
// in the project
#include "./inc/socket_async_os.h"

#include "./inc/dns_async.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/gballoc.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/crt_abstractions.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/xlogging.h"

// EXTRACT_IPV4 pulls the uint32_t IPv4 address out of an addrinfo struct
// This will not be needed for the asynchronous design
#ifdef WIN32_NOT_USED
//#define EXTRACT_IPV4(ptr) ((struct sockaddr_in *) ptr->ai_addr)->sin_addr.S_un.S_addr
#else
// The default definition handles lwIP. Please add comments for other systems tested.
#define EXTRACT_IPV4(ptr) ((struct sockaddr_in *) ptr->ai_addr)->sin_addr.s_addr
#endif

typedef struct
{
    char* hostname;
    uint32_t ip_v4;
    bool is_complete;
    bool is_failed;
} DNS_ASYNC_INSTANCE;

DNS_ASYNC_HANDLE dns_async_create(const char* hostname, DNS_ASYNC_OPTIONS* options)
{
    /* Codes_SRS_DNS_ASYNC_30_012: [ The optional options parameter shall be ignored. ]*/
    DNS_ASYNC_INSTANCE* result;
    (void)options;
    if (hostname == NULL)
    {
        /* Codes_SRS_DNS_ASYNC_30_011: [ If the hostname parameter is NULL, dns_async_create shall log an error and return NULL. ]*/
        LogError("NULL hostname");
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(DNS_ASYNC_INSTANCE));
        if (result == NULL)
        {
            /* Codes_SRS_DNS_ASYNC_30_014: [ On any failure, dns_async_create shall log an error and return NULL. ]*/
            LogError("malloc instance failed");
            result = NULL;
        }
        else
        {
			int ms_result;
            result->is_complete = false;
            result->is_failed = false;
            result->ip_v4 = 0;
            /* Codes_SRS_DNS_ASYNC_30_010: [ dns_async_create shall make a copy of the hostname parameter to allow immediate deletion by the caller. ]*/
            ms_result = mallocAndStrcpy_s(&result->hostname, hostname);
            if (ms_result != 0)
            {
                /* Codes_SRS_DNS_ASYNC_30_014: [ On any failure, dns_async_create shall log an error and return NULL. ]*/
                free(result);
                result = NULL;
            }
        }
    }
    return result;
}

/* Codes_SRS_DNS_ASYNC_30_021: [ dns_async_is_create_complete shall perform the asynchronous work of DNS lookup and log any errors. ]*/
bool dns_async_is_lookup_complete(DNS_ASYNC_HANDLE dns_in)
{
    DNS_ASYNC_INSTANCE* dns = (DNS_ASYNC_INSTANCE*)dns_in;

    bool result;
    if (dns == NULL)
    {
        /* Codes_SRS_DNS_ASYNC_30_020: [ If the dns parameter is NULL, dns_async_is_create_complete shall log an error and return false. ]*/
        LogError("NULL dns");
        result = false;
    }
    else
    {
        if (dns->is_complete)
        {
            /* Codes_SRS_DNS_ASYNC_30_024: [ If dns_async_is_create_complete has previously returned true, dns_async_is_create_complete shall do nothing and return true. ]*/
            result = true;
        }
        else
        {
            struct addrinfo *addrInfo = NULL;
            struct addrinfo *ptr = NULL;
            struct addrinfo hints;
			int getAddrResult;

			/* Codes_SRS_DNS_ASYNC_30_021: [ dns_async_is_create_complete shall perform the asynchronous work of DNS lookup and log any errors. ]*/
            // Only make one attempt at lookup for this
            // synchronous implementation
            dns->is_complete = true;

            //--------------------------------
            // Setup the hints address info structure
            // which is passed to the getaddrinfo() function
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            //--------------------------------
            // Call getaddrinfo(). If the call succeeds,
            // the result variable will hold a linked list
            // of addrinfo structures containing response
            // information
            getAddrResult = getaddrinfo(dns->hostname, NULL, &hints, &addrInfo);
            if (getAddrResult == 0)
            {
                // If we find the AF_INET address, use it as the return value
                for (ptr = addrInfo; ptr != NULL; ptr = ptr->ai_next)
                {
                    switch (ptr->ai_family)
                    {
                    case AF_INET:
                        /* Codes_SRS_DNS_ASYNC_30_032: [ If dns_async_is_create_complete has returned true and the lookup process has succeeded, dns_async_get_ipv4 shall return the discovered IPv4 address. ]*/
                        dns->ip_v4 = EXTRACT_IPV4(ptr);
                        break;
                    }
                }
                /* Codes_SRS_DNS_ASYNC_30_033: [ If dns_async_is_create_complete has returned true and the lookup process has failed, dns_async_get_ipv4 shall return 0. ]*/
                dns->is_failed = (dns->ip_v4 == 0);
                freeaddrinfo(addrInfo);
            }
            else
            {
                /* Codes_SRS_DNS_ASYNC_30_033: [ If dns_async_is_create_complete has returned true and the lookup process has failed, dns_async_get_ipv4 shall return 0. ]*/
                LogInfo("Failed DNS lookup for %s: %d", dns->hostname, getAddrResult);
                dns->is_failed = true;
            }
            // This synchronous implementation is incapable of being incomplete, so SRS_DNS_ASYNC_30_023 does not ever happen
            /* Codes_SRS_DNS_ASYNC_30_023: [ If the DNS lookup process is not yet complete, dns_async_is_create_complete shall return false. ]*/
            /* Codes_SRS_DNS_ASYNC_30_022: [ If the DNS lookup process has completed, dns_async_is_create_complete shall return true. ]*/
            result = true;
        }
    }

    return result;
}

void dns_async_destroy(DNS_ASYNC_HANDLE dns_in)
{
    DNS_ASYNC_INSTANCE* dns = (DNS_ASYNC_INSTANCE*)dns_in;
    if (dns == NULL)
    {
        /* Codes_SRS_DNS_ASYNC_30_050: [ If the dns parameter is NULL, dns_async_destroy shall log an error and do nothing. ]*/
        LogError("NULL dns");
    }
    else
    {
        /* Codes_SRS_DNS_ASYNC_30_051: [ dns_async_destroy shall delete all acquired resources and delete the DNS_ASYNC_HANDLE. ]*/
        free(dns->hostname);
        free(dns);
    }
}

uint32_t dns_async_get_ipv4(DNS_ASYNC_HANDLE dns_in)
{
    DNS_ASYNC_INSTANCE* dns = (DNS_ASYNC_INSTANCE*)dns_in;
    uint32_t result;
    if (dns == NULL)
    {
        /* Codes_SRS_DNS_ASYNC_30_030: [ If the dns parameter is NULL, dns_async_get_ipv4 shall log an error and return 0. ]*/
        LogError("NULL dns");
        result = 0;
    }
    else
    {
        if (dns->is_complete)
        {
            if (dns->is_failed)
            {
                /* Codes_SRS_DNS_ASYNC_30_033: [ If dns_async_is_create_complete has returned true and the lookup process has failed, dns_async_get_ipv4 shall return 0. ]*/
                result = 0;
            }
            else
            {
                /* Codes_SRS_DNS_ASYNC_30_032: [ If dns_async_is_create_complete has returned true and the lookup process has succeeded, dns_async_get_ipv4 shall return the discovered IPv4 address. ]*/
                result = dns->ip_v4;
            }
        }
        else
        {
            /* Codes_SRS_DNS_ASYNC_30_031: [ If dns_async_is_create_complete has not yet returned true, dns_async_get_ipv4 shall log an error and return 0. ]*/
            LogError("dns_async_get_ipv4 when not complete");
            result = 0;
        }
    }
    return result;
}

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file ssl_socket.h
 *	@brief	 Implements socket creation for TLSIO adapters.
 */

#ifndef AZURE_IOT_DNS_H
#define AZURE_IOT_DNS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "az_iot/c-utility/inc/azure_c_shared_utility/macro_utils.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"

    typedef void* DNS_ASYNC_HANDLE;

    // If options are added in future, DNS_ASYNC_OPTIONS will become a struct containing the options
    typedef void DNS_ASYNC_OPTIONS;

    /**
    * @brief	Begin the process of an asynchronous DNS lookup.
    *
    * @param   hostname	The url of the host to look up.
    *
    * @return	@c The newly created DNS_ASYNC_HANDLE.
    */
    MOCKABLE_FUNCTION(, DNS_ASYNC_HANDLE, dns_async_create, const char*, hostname, DNS_ASYNC_OPTIONS*, options);

    /**
    * @brief	Continue the lookup process and report its completion state. Must be polled repeatedly for completion.
    *
    * @param   dns	The DNS_ASYNC_HANDLE.
    *
    * @return	@c A bool to indicate completion.
    */
    MOCKABLE_FUNCTION(, bool, dns_async_is_lookup_complete, DNS_ASYNC_HANDLE, dns);

    /**
    * @brief	Return the IPv4 of a completed lookup process. Call only after dns_async_is_lookup_complete indicates completion.
    *
    * @param   dns	The DNS_ASYNC_HANDLE.
    *
    * @return	@c A uint32_t IPv4 address. 0 indicates failure or not finished.
    */
    MOCKABLE_FUNCTION(, uint32_t, dns_async_get_ipv4, DNS_ASYNC_HANDLE, dns);

    /**
    * @brief	Destroy the module.
    *
    * @param   dns	The DNS_ASYNC_HANDLE.
    */
    MOCKABLE_FUNCTION(, void, dns_async_destroy, DNS_ASYNC_HANDLE, dns);


#ifdef __cplusplus
}
#endif

#endif /* AZURE_IOT_DNS_H */

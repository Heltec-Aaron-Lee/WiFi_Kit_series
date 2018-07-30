// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_client_version.h
*	@brief Functions for managing the client SDK version.
*/

#ifndef IOTHUB_CLIENT_DPS_CONN_H
#define IOTHUB_CLIENT_DPS_CONN_H

#include "az_iot/c-utility/inc/azure_c_shared_utility/macro_utils.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"

#ifdef USE_DPS_MODULE
#include "az_iot/c-utility/inc/azure_hub_modules/iothub_device_auth.h"
#endif

#include "iothub_client_ll.h"

#ifdef __cplusplus
extern "C"
{
#endif

     /**
     * @brief	Creates a IoT Hub client for communication with an existing IoT
     * 			Hub using the device auth module.
     *
     * @param	iothub_uri	Pointer to an ioThub hostname received in the DPS process
     * @param	device_id	Pointer to the device Id of the device
     * @param	device_auth_handle	a device auth handle used to generate the connection string
     * @param	protocol			Function pointer for protocol implementation
     *
     * @return	A non-NULL @c IOTHUB_CLIENT_LL_HANDLE value that is used when
     * 			invoking other functions for IoT Hub client and @c NULL on failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_LL_HANDLE, IoTHubClient_LL_CreateFromDeviceAuth, const char*, iothub_uri, const char*, device_id, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

#ifdef __cplusplus
}
#endif

#endif // IOTHUB_CLIENT_DPS_CONN_H

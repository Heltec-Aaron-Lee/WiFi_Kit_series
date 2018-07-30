// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_TRANSPORT_H
#define IOTHUB_TRANSPORT_H

typedef struct TRANSPORT_HANDLE_DATA_TAG* TRANSPORT_HANDLE;


#include "az_iot/c-utility/inc/azure_c_shared_utility/lock.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/crt_abstractions.h"

#include "iothub_client_ll.h"
#include "iothub_client_private.h"
#include "iothub_transport_ll.h"

#ifndef IOTHUB_CLIENT_INSTANCE_TYPE
typedef struct IOTHUB_CLIENT_INSTANCE_TAG* IOTHUB_CLIENT_HANDLE;
#define IOTHUB_CLIENT_INSTANCE_TYPE
#endif // IOTHUB_CLIENT_INSTANCE

#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"

typedef void(*IOTHUB_CLIENT_MULTIPLEXED_DO_WORK)(void* iotHubClientInstance);

#ifdef __cplusplus
extern "C"
{
#endif

    MOCKABLE_FUNCTION(, TRANSPORT_HANDLE, IoTHubTransport_Create, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol, const char*, iotHubName, const char*, iotHubSuffix);
    MOCKABLE_FUNCTION(, void, IoTHubTransport_Destroy, TRANSPORT_HANDLE, transportHandle);
    MOCKABLE_FUNCTION(, LOCK_HANDLE, IoTHubTransport_GetLock, TRANSPORT_HANDLE, transportHandle);
    MOCKABLE_FUNCTION(, TRANSPORT_LL_HANDLE, IoTHubTransport_GetLLTransport, TRANSPORT_HANDLE, transportHandle);
    MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubTransport_StartWorkerThread, TRANSPORT_HANDLE, transportHandle, IOTHUB_CLIENT_HANDLE, clientHandle, IOTHUB_CLIENT_MULTIPLEXED_DO_WORK, muxDoWork);
    MOCKABLE_FUNCTION(, bool, IoTHubTransport_SignalEndWorkerThread, TRANSPORT_HANDLE, transportHandle, IOTHUB_CLIENT_HANDLE, clientHandle);
    MOCKABLE_FUNCTION(, void, IoTHubTransport_JoinWorkerThread, TRANSPORT_HANDLE, transportHandle, IOTHUB_CLIENT_HANDLE, clientHandle);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_TRANSPORT_H */

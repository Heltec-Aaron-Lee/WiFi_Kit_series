// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_TRANSPORT_LL_H
#define IOTHUB_TRANSPORT_LL_H


typedef void* TRANSPORT_LL_HANDLE;
typedef void* IOTHUB_DEVICE_HANDLE;

struct TRANSPORT_PROVIDER_TAG;
typedef struct TRANSPORT_PROVIDER_TAG TRANSPORT_PROVIDER;

union IOTHUB_IDENTITY_INFO_TAG;
typedef union IOTHUB_IDENTITY_INFO_TAG IOTHUB_IDENTITY_INFO;

typedef void* METHOD_HANDLE;

#include "az_iot/c-utility/inc/azure_c_shared_utility/doublylinkedlist.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/strings.h"
#include "iothub_message.h"
#include "iothub_client_authorization.h"

struct MESSAGE_DISPOSITION_CONTEXT_TAG;
typedef struct MESSAGE_DISPOSITION_CONTEXT_TAG* MESSAGE_DISPOSITION_CONTEXT_HANDLE;
typedef struct MESSAGE_CALLBACK_INFO_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    MESSAGE_DISPOSITION_CONTEXT_HANDLE transportContext;
}MESSAGE_CALLBACK_INFO;

#include "iothub_client_ll.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief	This struct captures device configuration. */
    typedef struct IOTHUB_DEVICE_CONFIG_TAG
    {
        /** @brief	A string that identifies the device. */
        const char* deviceId;

        /** @brief	The device key used to authenticate the device. */
        const char* deviceKey;

        /** @brief	The device SAS used to authenticate the device in place of using the device key. */
        const char* deviceSasToken;

        IOTHUB_AUTHORIZATION_HANDLE authorization_module; // with either SAS Token, x509 Certs, and Device SAS Token
    } IOTHUB_DEVICE_CONFIG;

    typedef STRING_HANDLE (*pfIoTHubTransport_GetHostname)(TRANSPORT_LL_HANDLE handle);
    typedef IOTHUB_CLIENT_RESULT(*pfIoTHubTransport_SetOption)(TRANSPORT_LL_HANDLE handle, const char *optionName, const void* value);
    typedef TRANSPORT_LL_HANDLE(*pfIoTHubTransport_Create)(const IOTHUBTRANSPORT_CONFIG* config);
    typedef void (*pfIoTHubTransport_Destroy)(TRANSPORT_LL_HANDLE handle);
    typedef IOTHUB_DEVICE_HANDLE(*pfIotHubTransport_Register)(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, PDLIST_ENTRY waitingToSend);
    typedef void(*pfIotHubTransport_Unregister)(IOTHUB_DEVICE_HANDLE deviceHandle);
    typedef int (*pfIoTHubTransport_Subscribe)(IOTHUB_DEVICE_HANDLE handle);
    typedef void (*pfIoTHubTransport_Unsubscribe)(IOTHUB_DEVICE_HANDLE handle);
    typedef void (*pfIoTHubTransport_DoWork)(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle);
    typedef int(*pfIoTHubTransport_SetRetryPolicy)(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds);
    typedef IOTHUB_CLIENT_RESULT(*pfIoTHubTransport_GetSendStatus)(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus);
    typedef int (*pfIoTHubTransport_Subscribe_DeviceTwin)(IOTHUB_DEVICE_HANDLE handle);
    typedef void (*pfIoTHubTransport_Unsubscribe_DeviceTwin)(IOTHUB_DEVICE_HANDLE handle);
    typedef IOTHUB_CLIENT_RESULT(*pfIotHubTransport_SendMessageDisposition)(MESSAGE_CALLBACK_INFO* messageData, IOTHUBMESSAGE_DISPOSITION_RESULT disposition);
    typedef IOTHUB_PROCESS_ITEM_RESULT(*pfIoTHubTransport_ProcessItem)(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item);
    typedef int(*pfIoTHubTransport_Subscribe_DeviceMethod)(IOTHUB_DEVICE_HANDLE handle);
    typedef void(*pfIoTHubTransport_Unsubscribe_DeviceMethod)(IOTHUB_DEVICE_HANDLE handle);
    typedef int(*pfIoTHubTransport_DeviceMethod_Response)(IOTHUB_DEVICE_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response);

#define TRANSPORT_PROVIDER_FIELDS                                                   \
pfIotHubTransport_SendMessageDisposition IoTHubTransport_SendMessageDisposition;  \
pfIoTHubTransport_Subscribe_DeviceMethod IoTHubTransport_Subscribe_DeviceMethod;    \
pfIoTHubTransport_Unsubscribe_DeviceMethod IoTHubTransport_Unsubscribe_DeviceMethod;\
pfIoTHubTransport_DeviceMethod_Response IoTHubTransport_DeviceMethod_Response;      \
pfIoTHubTransport_Subscribe_DeviceTwin IoTHubTransport_Subscribe_DeviceTwin;        \
pfIoTHubTransport_Unsubscribe_DeviceTwin IoTHubTransport_Unsubscribe_DeviceTwin;    \
pfIoTHubTransport_ProcessItem IoTHubTransport_ProcessItem;                          \
pfIoTHubTransport_GetHostname IoTHubTransport_GetHostname;                          \
pfIoTHubTransport_SetOption IoTHubTransport_SetOption;                              \
pfIoTHubTransport_Create IoTHubTransport_Create;                                    \
pfIoTHubTransport_Destroy IoTHubTransport_Destroy;                                  \
pfIotHubTransport_Register IoTHubTransport_Register;                                \
pfIotHubTransport_Unregister IoTHubTransport_Unregister;                            \
pfIoTHubTransport_Subscribe IoTHubTransport_Subscribe;                              \
pfIoTHubTransport_Unsubscribe IoTHubTransport_Unsubscribe;                          \
pfIoTHubTransport_DoWork IoTHubTransport_DoWork;                                    \
pfIoTHubTransport_SetRetryPolicy IoTHubTransport_SetRetryPolicy;                    \
pfIoTHubTransport_GetSendStatus IoTHubTransport_GetSendStatus  /*there's an intentional missing ; on this line*/

    struct TRANSPORT_PROVIDER_TAG
    {
        TRANSPORT_PROVIDER_FIELDS;
    };

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_TRANSPORT_LL_H */

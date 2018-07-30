// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBTRANSPORT_MQTT_COMMON_H
#define IOTHUBTRANSPORT_MQTT_COMMON_H

#include "../inc/iothub_transport_ll.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MQTT_TRANSPORT_PROXY_OPTIONS_TAG
{
    const char* host_address;
    int port;
    const char* username;
    const char* password;
} MQTT_TRANSPORT_PROXY_OPTIONS;

typedef XIO_HANDLE(*MQTT_GET_IO_TRANSPORT)(const char* fully_qualified_name, const MQTT_TRANSPORT_PROXY_OPTIONS* mqtt_transport_proxy_options);

MOCKABLE_FUNCTION(, TRANSPORT_LL_HANDLE, IoTHubTransport_MQTT_Common_Create, const IOTHUBTRANSPORT_CONFIG*,  config, MQTT_GET_IO_TRANSPORT, get_io_transport);
MOCKABLE_FUNCTION(, void, IoTHubTransport_MQTT_Common_Destroy, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, int, IoTHubTransport_MQTT_Common_Subscribe, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, IoTHubTransport_MQTT_Common_Unsubscribe, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod, IOTHUB_DEVICE_HANDLE, handle);
MOCKABLE_FUNCTION(, int, IoTHubTransport_MQTT_Common_DeviceMethod_Response, IOTHUB_DEVICE_HANDLE, handle, METHOD_HANDLE, methodId, const unsigned char*, response, size_t, response_size, int, status_response);
MOCKABLE_FUNCTION(, IOTHUB_PROCESS_ITEM_RESULT, IoTHubTransport_MQTT_Common_ProcessItem, TRANSPORT_LL_HANDLE, handle, IOTHUB_IDENTITY_TYPE, item_type, IOTHUB_IDENTITY_INFO*, iothub_item);
MOCKABLE_FUNCTION(, void, IoTHubTransport_MQTT_Common_DoWork, TRANSPORT_LL_HANDLE, handle, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubTransport_MQTT_Common_GetSendStatus, IOTHUB_DEVICE_HANDLE, handle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubTransport_MQTT_Common_SetOption, TRANSPORT_LL_HANDLE, handle, const char*, option, const void*, value);
MOCKABLE_FUNCTION(, IOTHUB_DEVICE_HANDLE, IoTHubTransport_MQTT_Common_Register, TRANSPORT_LL_HANDLE, handle, const IOTHUB_DEVICE_CONFIG*, device, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, PDLIST_ENTRY, waitingToSend);
MOCKABLE_FUNCTION(, void, IoTHubTransport_MQTT_Common_Unregister, IOTHUB_DEVICE_HANDLE, deviceHandle);
MOCKABLE_FUNCTION(, int, IoTHubTransport_MQTT_Common_SetRetryPolicy, TRANSPORT_LL_HANDLE, handle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitInSeconds);
MOCKABLE_FUNCTION(, STRING_HANDLE, IoTHubTransport_MQTT_Common_GetHostname, TRANSPORT_LL_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubTransport_MQTT_Common_SendMessageDisposition, MESSAGE_CALLBACK_INFO*, message_data, IOTHUBMESSAGE_DISPOSITION_RESULT, disposition)


#ifdef __cplusplus
}
#endif

#endif /* IOTHUBTRANSPORT_MQTT_COMMON_H */

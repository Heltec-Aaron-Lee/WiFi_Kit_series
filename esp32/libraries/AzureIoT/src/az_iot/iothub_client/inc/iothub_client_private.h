// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_CLIENT_PRIVATE_H
#define IOTHUB_CLIENT_PRIVATE_H

#include <signal.h>

#include "az_iot/c-utility/inc/azure_c_shared_utility/constbuffer.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/crt_abstractions.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/doublylinkedlist.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/macro_utils.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/tickcounter.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"

#include "iothub_message.h"
#include "iothub_client_ll.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define EVENT_ENDPOINT "/messages/events"
#define MESSAGE_ENDPOINT "/messages/devicebound"
#define MESSAGE_ENDPOINT_HTTP "/messages/devicebound"
#define MESSAGE_ENDPOINT_HTTP_ETAG "/messages/devicebound/"
#define CLIENT_DEVICE_TYPE_PREFIX "iothubclient"
#define CLIENT_DEVICE_BACKSLASH "/"
#define CBS_REPLY_TO "cbs"
#define CBS_ENDPOINT "/$" CBS_REPLY_TO
#define API_VERSION "?api-version=2016-11-14"
#define REJECT_QUERY_PARAMETER "&reject"

typedef bool(*IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX)(MESSAGE_CALLBACK_INFO* messageData, void* userContextCallback);

MOCKABLE_FUNCTION(, void, IoTHubClient_LL_SendComplete, IOTHUB_CLIENT_LL_HANDLE, handle, PDLIST_ENTRY, completed, IOTHUB_CLIENT_CONFIRMATION_RESULT, result);
MOCKABLE_FUNCTION(, void, IoTHubClient_LL_ReportedStateComplete, IOTHUB_CLIENT_LL_HANDLE, handle, uint32_t, item_id, int, status_code);
MOCKABLE_FUNCTION(, bool, IoTHubClient_LL_MessageCallback, IOTHUB_CLIENT_LL_HANDLE, handle, MESSAGE_CALLBACK_INFO*, message_data);
MOCKABLE_FUNCTION(, void, IoTHubClient_LL_RetrievePropertyComplete, IOTHUB_CLIENT_LL_HANDLE, handle, DEVICE_TWIN_UPDATE_STATE, update_state, const unsigned char*, payLoad, size_t, size);
MOCKABLE_FUNCTION(, int, IoTHubClient_LL_DeviceMethodComplete, IOTHUB_CLIENT_LL_HANDLE, handle, const char*, method_name, const unsigned char*, payLoad, size_t, size, METHOD_HANDLE, response_id);
MOCKABLE_FUNCTION(, void, IoTHubClient_LL_ConnectionStatusCallBack, IOTHUB_CLIENT_LL_HANDLE, handle, IOTHUB_CLIENT_CONNECTION_STATUS, status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SetMessageCallback_Ex, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX, messageCallback, void*, userContextCallback);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SendMessageDisposition, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, MESSAGE_CALLBACK_INFO*, messageData, IOTHUBMESSAGE_DISPOSITION_RESULT, disposition);
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_GetOption, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, const char*, optionName, void**, value);

typedef struct IOTHUB_MESSAGE_LIST_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;
    void* context; 
    DLIST_ENTRY entry;
    tickcounter_ms_t ms_timesOutAfter; /* a value of "0" means "no timeout", if the IOTHUBCLIENT_LL's handle tickcounter > msTimesOutAfer then the message shall timeout*/
}IOTHUB_MESSAGE_LIST;

typedef struct IOTHUB_DEVICE_TWIN_TAG
{
    uint32_t item_id;
    tickcounter_ms_t ms_timesOutAfter; /* a value of "0" means "no timeout", if the IOTHUBCLIENT_LL's handle tickcounter > msTimesOutAfer then the message shall timeout*/
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reported_state_callback;
    CONSTBUFFER_HANDLE report_data_handle;
    void* context;
    DLIST_ENTRY entry;
	IOTHUB_CLIENT_LL_HANDLE client_handle;
	IOTHUB_DEVICE_HANDLE device_handle;
} IOTHUB_DEVICE_TWIN;

union IOTHUB_IDENTITY_INFO_TAG
{
    IOTHUB_DEVICE_TWIN* device_twin;
    IOTHUB_MESSAGE_LIST* iothub_message;
};

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_PRIVATE_H */

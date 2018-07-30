// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h> 
#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/gballoc.h"

#include <signal.h>
#include <stddef.h>
#include "az_iot/c-utility/inc/azure_c_shared_utility/optimize_size.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/crt_abstractions.h"
#include "../inc/iothub_client.h"
#include "../inc/iothub_client_ll.h"
#include "../inc/iothub_client_private.h"
#include "../inc/iothubtransport.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/threadapi.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/lock.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/xlogging.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/singlylinkedlist.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/vector.h"

struct IOTHUB_QUEUE_CONTEXT_TAG;

typedef struct IOTHUB_CLIENT_INSTANCE_TAG
{
    IOTHUB_CLIENT_LL_HANDLE IoTHubClientLLHandle;
    TRANSPORT_HANDLE TransportHandle;
    THREAD_HANDLE ThreadHandle;
    LOCK_HANDLE LockHandle;
    sig_atomic_t StopThread;
#ifdef USE_UPLOADTOBLOB
    SINGLYLINKEDLIST_HANDLE savedDataToBeCleaned; /*list containing UPLOADTOBLOB_SAVED_DATA*/
#endif
    int created_with_transport_handle;
    VECTOR_HANDLE saved_user_callback_list;
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK desired_state_callback;
    IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK event_confirm_callback;
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reported_state_callback;
    IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connection_status_callback;
    IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC device_method_callback;
    IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inbound_device_method_callback;
    IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC message_callback;
    struct IOTHUB_QUEUE_CONTEXT_TAG* devicetwin_user_context;
    struct IOTHUB_QUEUE_CONTEXT_TAG* connection_status_user_context;
    struct IOTHUB_QUEUE_CONTEXT_TAG* message_user_context;
    struct IOTHUB_QUEUE_CONTEXT_TAG* method_user_context;
} IOTHUB_CLIENT_INSTANCE;

#ifdef USE_UPLOADTOBLOB
typedef struct UPLOADTOBLOB_SAVED_DATA_TAG
{
    unsigned char* source;
    size_t size;
    char* destinationFileName;
    IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK iotHubClientFileUploadCallback;
    void* context;
    THREAD_HANDLE uploadingThreadHandle;
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    LOCK_HANDLE lockGarbage;
    int canBeGarbageCollected; /*flag indicating that the UPLOADTOBLOB_SAVED_DATA structure can be freed because the thread deadling with it finished*/
}UPLOADTOBLOB_SAVED_DATA;
#endif

#define USER_CALLBACK_TYPE_VALUES       \
    CALLBACK_TYPE_DEVICE_TWIN,          \
    CALLBACK_TYPE_EVENT_CONFIRM,        \
    CALLBACK_TYPE_REPORTED_STATE,       \
    CALLBACK_TYPE_CONNECTION_STATUS,    \
    CALLBACK_TYPE_DEVICE_METHOD,        \
    CALLBACK_TYPE_INBOUD_DEVICE_METHOD, \
    CALLBACK_TYPE_MESSAGE

DEFINE_ENUM(USER_CALLBACK_TYPE, USER_CALLBACK_TYPE_VALUES)
DEFINE_ENUM_STRINGS(USER_CALLBACK_TYPE, USER_CALLBACK_TYPE_VALUES)

typedef struct DEVICE_TWIN_CALLBACK_INFO_TAG
{
    DEVICE_TWIN_UPDATE_STATE update_state;
    unsigned char* payLoad;
    size_t size;
} DEVICE_TWIN_CALLBACK_INFO;

typedef struct EVENT_CONFIRM_CALLBACK_INFO_TAG
{
    IOTHUB_CLIENT_CONFIRMATION_RESULT confirm_result;
} EVENT_CONFIRM_CALLBACK_INFO;

typedef struct REPORTED_STATE_CALLBACK_INFO_TAG
{
    int status_code;
} REPORTED_STATE_CALLBACK_INFO;

typedef struct CONNECTION_STATUS_CALLBACK_INFO_TAG
{
    IOTHUB_CLIENT_CONNECTION_STATUS connection_status;
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON status_reason;
} CONNECTION_STATUS_CALLBACK_INFO;

typedef struct METHOD_CALLBACK_INFO_TAG
{
    STRING_HANDLE method_name;
    BUFFER_HANDLE payload;
    METHOD_HANDLE method_id;
} METHOD_CALLBACK_INFO;

typedef struct USER_CALLBACK_INFO_TAG
{
    USER_CALLBACK_TYPE type;
    void* userContextCallback;
    union IOTHUB_CALLBACK
    {
        DEVICE_TWIN_CALLBACK_INFO dev_twin_cb_info;
        EVENT_CONFIRM_CALLBACK_INFO event_confirm_cb_info;
        REPORTED_STATE_CALLBACK_INFO reported_state_cb_info;
        CONNECTION_STATUS_CALLBACK_INFO connection_status_cb_info;
        METHOD_CALLBACK_INFO method_cb_info;
        MESSAGE_CALLBACK_INFO* message_cb_info;
    } iothub_callback;
} USER_CALLBACK_INFO;

typedef struct IOTHUB_QUEUE_CONTEXT_TAG
{
    IOTHUB_CLIENT_INSTANCE* iotHubClientHandle;
    void* userContextCallback;
} IOTHUB_QUEUE_CONTEXT;

/*used by unittests only*/
const size_t IoTHubClient_ThreadTerminationOffset = offsetof(IOTHUB_CLIENT_INSTANCE, StopThread);

#ifdef USE_UPLOADTOBLOB
/*this function is called from _Destroy and from ScheduleWork_Thread to join finished blobUpload threads and free that memory*/
static void garbageCollectorImpl(IOTHUB_CLIENT_INSTANCE* iotHubClientInstance)
{
    /*see if any savedData structures can be disposed of*/
    /*Codes_SRS_IOTHUBCLIENT_02_072: [ All threads marked as disposable (upon completion of a file upload) shall be joined and the data structures build for them shall be freed. ]*/
    LIST_ITEM_HANDLE item = singlylinkedlist_get_head_item(iotHubClientInstance->savedDataToBeCleaned);
    while (item != NULL)
    {
        const UPLOADTOBLOB_SAVED_DATA* savedData = (const UPLOADTOBLOB_SAVED_DATA*)singlylinkedlist_item_get_value(item);
        LIST_ITEM_HANDLE old_item = item;
        item = singlylinkedlist_get_next_item(item);

        if (Lock(savedData->lockGarbage) != LOCK_OK)
        {
            LogError("unable to Lock");
        }
        else
        {
            if (savedData->canBeGarbageCollected == 1)
            {
                int notUsed;
                if (ThreadAPI_Join(savedData->uploadingThreadHandle, &notUsed) != THREADAPI_OK)
                {
                    LogError("unable to ThreadAPI_Join");
                }
                (void)singlylinkedlist_remove(iotHubClientInstance->savedDataToBeCleaned, old_item);
                free((void*)savedData->source);
                free((void*)savedData->destinationFileName);

                if (Unlock(savedData->lockGarbage) != LOCK_OK)
                {
                    LogError("unable to unlock after locking");
                }
                (void)Lock_Deinit(savedData->lockGarbage);
                free((void*)savedData);
            }
            else
            {
                if (Unlock(savedData->lockGarbage) != LOCK_OK)
                {
                    LogError("unable to unlock after locking");
                }
            }
        }
    }
}
#endif

static bool iothub_ll_message_callback(MESSAGE_CALLBACK_INFO* messageData, void* userContextCallback)
{
    bool result;
    IOTHUB_QUEUE_CONTEXT* queue_context = (IOTHUB_QUEUE_CONTEXT*)userContextCallback;
    if (queue_context == NULL)
    {
        LogError("invalid parameter userContextCallback(NULL)");
        result = false;
    }
    else
    {
        USER_CALLBACK_INFO queue_cb_info;
        queue_cb_info.type = CALLBACK_TYPE_MESSAGE;
        queue_cb_info.userContextCallback = queue_context->userContextCallback;
        queue_cb_info.iothub_callback.message_cb_info = messageData;
        if (VECTOR_push_back(queue_context->iotHubClientHandle->saved_user_callback_list, &queue_cb_info, 1) == 0)
        {
            result = true;
        }
        else
        {
            LogError("message callback vector push failed.");
            result = false;
        }
    }
    return result;
}

static int make_method_calback_queue_context(USER_CALLBACK_INFO* queue_cb_info, const char* method_name, const unsigned char* payload, size_t size, METHOD_HANDLE method_id, IOTHUB_QUEUE_CONTEXT* queue_context)
{
    int result;
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [ IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall copy the method_name and payload. ] */
    queue_cb_info->userContextCallback = queue_context->userContextCallback;
    queue_cb_info->iothub_callback.method_cb_info.method_id = method_id;
    if ((queue_cb_info->iothub_callback.method_cb_info.method_name = STRING_construct(method_name)) == NULL)
    {
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [ If a failure is encountered IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a non-NULL value. ]*/
        LogError("STRING_construct failed");
        result = __FAILURE__;
    }
    else
    {
        if ((queue_cb_info->iothub_callback.method_cb_info.payload = BUFFER_create(payload, size)) == NULL)
        {
            STRING_delete(queue_cb_info->iothub_callback.method_cb_info.method_name);
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [ If a failure is encountered IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a non-NULL value. ]*/
            LogError("BUFFER_create failed");
            result = __FAILURE__;
        }
        else
        {
            if (VECTOR_push_back(queue_context->iotHubClientHandle->saved_user_callback_list, queue_cb_info, 1) == 0)
            {
                result = 0;
            }
            else
            {
                STRING_delete(queue_cb_info->iothub_callback.method_cb_info.method_name);
                BUFFER_delete(queue_cb_info->iothub_callback.method_cb_info.payload);
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [ If a failure is encountered IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a non-NULL value. ]*/
                LogError("VECTOR_push_back failed");
                result = __FAILURE__;
            }
        }
    }
    return result;
}

static int iothub_ll_device_method_callback(const char* method_name, const unsigned char* payload, size_t size, METHOD_HANDLE method_id, void* userContextCallback)
{
    int result;
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_001: [ if userContextCallback is NULL, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a nonNULL value. ] */
    if (userContextCallback == NULL)
    {
        LogError("invalid parameter userContextCallback(NULL)");
        result = __FAILURE__;
    }
    else
    {
        IOTHUB_QUEUE_CONTEXT* queue_context = (IOTHUB_QUEUE_CONTEXT*)userContextCallback;

        USER_CALLBACK_INFO queue_cb_info;
        queue_cb_info.type = CALLBACK_TYPE_DEVICE_METHOD;

        result = make_method_calback_queue_context(&queue_cb_info, method_name, payload, size, method_id, queue_context);
        if (result != 0)
        {
            LogError("construction of method calback queue context failed");
            result = __FAILURE__;
        }
    }
    return result;
}

static int iothub_ll_inbound_device_method_callback(const char* method_name, const unsigned char* payload, size_t size, METHOD_HANDLE method_id, void* userContextCallback)
{
    int result;
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_001: [ if userContextCallback is NULL, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK shall return a nonNULL value. ] */
    if (userContextCallback == NULL)
    {
        LogError("invalid parameter userContextCallback(NULL)");
        result = __FAILURE__;
    }
    else
    {
        IOTHUB_QUEUE_CONTEXT* queue_context = (IOTHUB_QUEUE_CONTEXT*)userContextCallback;

        USER_CALLBACK_INFO queue_cb_info;
        queue_cb_info.type = CALLBACK_TYPE_INBOUD_DEVICE_METHOD;

        result = make_method_calback_queue_context(&queue_cb_info, method_name, payload, size, method_id, queue_context);
        if (result != 0)
        {
            LogError("construction of method calback queue context failed");
            result = __FAILURE__;
        }
    }
    return result;
}

static void iothub_ll_connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    IOTHUB_QUEUE_CONTEXT* queue_context = (IOTHUB_QUEUE_CONTEXT*)userContextCallback;
    if (queue_context != NULL)
    {
        USER_CALLBACK_INFO queue_cb_info;
        queue_cb_info.type = CALLBACK_TYPE_CONNECTION_STATUS;
        queue_cb_info.userContextCallback = queue_context->userContextCallback;
        queue_cb_info.iothub_callback.connection_status_cb_info.status_reason = reason;
        queue_cb_info.iothub_callback.connection_status_cb_info.connection_status = result;
        if (VECTOR_push_back(queue_context->iotHubClientHandle->saved_user_callback_list, &queue_cb_info, 1) != 0)
        {
            LogError("connection status callback vector push failed.");
        }
    }
}

static void iothub_ll_event_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    IOTHUB_QUEUE_CONTEXT* queue_context = (IOTHUB_QUEUE_CONTEXT*)userContextCallback;
    if (queue_context != NULL)
    {
        USER_CALLBACK_INFO queue_cb_info;
        queue_cb_info.type = CALLBACK_TYPE_EVENT_CONFIRM;
        queue_cb_info.userContextCallback = queue_context->userContextCallback;
        queue_cb_info.iothub_callback.event_confirm_cb_info.confirm_result = result;
        if (VECTOR_push_back(queue_context->iotHubClientHandle->saved_user_callback_list, &queue_cb_info, 1) != 0)
        {
            LogError("event confirm callback vector push failed.");
        }
        free(queue_context);
    }
}

static void iothub_ll_reported_state_callback(int status_code, void* userContextCallback)
{
    IOTHUB_QUEUE_CONTEXT* queue_context = (IOTHUB_QUEUE_CONTEXT*)userContextCallback;
    if (queue_context != NULL)
    {
        USER_CALLBACK_INFO queue_cb_info;
        queue_cb_info.type = CALLBACK_TYPE_REPORTED_STATE;
        queue_cb_info.userContextCallback = queue_context->userContextCallback;
        queue_cb_info.iothub_callback.reported_state_cb_info.status_code = status_code;
        if (VECTOR_push_back(queue_context->iotHubClientHandle->saved_user_callback_list, &queue_cb_info, 1) != 0)
        {
            LogError("reported state callback vector push failed.");
        }
        free(queue_context);
    }
}

static void iothub_ll_device_twin_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    IOTHUB_QUEUE_CONTEXT* queue_context = (IOTHUB_QUEUE_CONTEXT*)userContextCallback;
    if (queue_context != NULL)
    {
        int push_to_vector;

        USER_CALLBACK_INFO queue_cb_info;
        queue_cb_info.type = CALLBACK_TYPE_DEVICE_TWIN;
        queue_cb_info.userContextCallback = queue_context->userContextCallback;
        queue_cb_info.iothub_callback.dev_twin_cb_info.update_state = update_state;
        if (payLoad == NULL)
        {
            queue_cb_info.iothub_callback.dev_twin_cb_info.payLoad = NULL;
            queue_cb_info.iothub_callback.dev_twin_cb_info.size = 0;
            push_to_vector = 0;
        }
        else
        {
            queue_cb_info.iothub_callback.dev_twin_cb_info.payLoad = (unsigned char*)malloc(size);
            if (queue_cb_info.iothub_callback.dev_twin_cb_info.payLoad == NULL)
            {
                LogError("failure allocating payload in device twin callback.");
                queue_cb_info.iothub_callback.dev_twin_cb_info.size = 0;
                push_to_vector = __FAILURE__;
            }
            else
            {
                (void)memcpy(queue_cb_info.iothub_callback.dev_twin_cb_info.payLoad, payLoad, size);
                queue_cb_info.iothub_callback.dev_twin_cb_info.size = size;
                push_to_vector = 0;
            }
        }
        if (push_to_vector == 0)
        {
            if (VECTOR_push_back(queue_context->iotHubClientHandle->saved_user_callback_list, &queue_cb_info, 1) != 0)
            {
                if (queue_cb_info.iothub_callback.dev_twin_cb_info.payLoad != NULL)
                {
                    free(queue_cb_info.iothub_callback.dev_twin_cb_info.payLoad);
                }
                LogError("device twin callback userContextCallback vector push failed.");
            }
        }
    }
    else
    {
        LogError("device twin callback userContextCallback NULL");
    }
}

static void dispatch_user_callbacks(IOTHUB_CLIENT_INSTANCE* iotHubClientInstance, VECTOR_HANDLE call_backs)
{
    size_t callbacks_length = VECTOR_size(call_backs);
    size_t index;
    for (index = 0; index < callbacks_length; index++)
    {
        USER_CALLBACK_INFO* queued_cb = (USER_CALLBACK_INFO*)VECTOR_element(call_backs, index);
        if (queued_cb == NULL)
        {
            LogError("VECTOR_element at index %zd is NULL.", index);
        }
        else
        {
            switch (queued_cb->type)
            {
                case CALLBACK_TYPE_DEVICE_TWIN:
                {
                    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK desired_state_callback;

                    if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
                    {
                        LogError("failed locking for dispatch_user_callbacks");
                        desired_state_callback = NULL;
                    }
                    else
                    {
                        desired_state_callback = iotHubClientInstance->desired_state_callback;
                        (void)Unlock(iotHubClientInstance->LockHandle);
                    }

                    if (desired_state_callback)
                    {
                        desired_state_callback(queued_cb->iothub_callback.dev_twin_cb_info.update_state, queued_cb->iothub_callback.dev_twin_cb_info.payLoad, queued_cb->iothub_callback.dev_twin_cb_info.size, queued_cb->userContextCallback);
                    }

                    if (queued_cb->iothub_callback.dev_twin_cb_info.payLoad)
                    {
                        free(queued_cb->iothub_callback.dev_twin_cb_info.payLoad);
                    }
                    break;
                }
                case CALLBACK_TYPE_EVENT_CONFIRM:
                    if (iotHubClientInstance->event_confirm_callback)
                    {
                        iotHubClientInstance->event_confirm_callback(queued_cb->iothub_callback.event_confirm_cb_info.confirm_result, queued_cb->userContextCallback);
                    }
                    break;
                case CALLBACK_TYPE_REPORTED_STATE:
                    if (iotHubClientInstance->reported_state_callback)
                    {
                        iotHubClientInstance->reported_state_callback(queued_cb->iothub_callback.reported_state_cb_info.status_code, queued_cb->userContextCallback);
                    }
                    break;
                case CALLBACK_TYPE_CONNECTION_STATUS:
                    if (iotHubClientInstance->connection_status_callback)
                    {
                        iotHubClientInstance->connection_status_callback(queued_cb->iothub_callback.connection_status_cb_info.connection_status, queued_cb->iothub_callback.connection_status_cb_info.status_reason, queued_cb->userContextCallback);
                    }
                    break;
                case CALLBACK_TYPE_DEVICE_METHOD:
                    if (iotHubClientInstance->device_method_callback)
                    {
                        const char* method_name = STRING_c_str(queued_cb->iothub_callback.method_cb_info.method_name);
                        const unsigned char* payload = BUFFER_u_char(queued_cb->iothub_callback.method_cb_info.payload);
                        size_t payload_len = BUFFER_length(queued_cb->iothub_callback.method_cb_info.payload);

                        unsigned char* payload_resp = NULL;
                        size_t response_size = 0;
                        int status = iotHubClientInstance->device_method_callback(method_name, payload, payload_len, &payload_resp, &response_size, queued_cb->userContextCallback);

                        if (payload_resp && (response_size > 0))
                        {
                            IOTHUB_CLIENT_HANDLE handle = iotHubClientInstance->method_user_context->iotHubClientHandle;
                            IOTHUB_CLIENT_RESULT result = IoTHubClient_DeviceMethodResponse(handle, queued_cb->iothub_callback.method_cb_info.method_id, (const unsigned char*)payload_resp, response_size, status);
                            if (result != IOTHUB_CLIENT_OK)
                            {
                                LogError("IoTHubClient_LL_DeviceMethodResponse failed");
                            }
                        }

                        BUFFER_delete(queued_cb->iothub_callback.method_cb_info.payload);
                        STRING_delete(queued_cb->iothub_callback.method_cb_info.method_name);
                        
                        if (payload_resp)
                        {
                            free(payload_resp);
                        }
                    }
                    break;
                case CALLBACK_TYPE_INBOUD_DEVICE_METHOD:
                    if (iotHubClientInstance->inbound_device_method_callback)
                    {
                        const char* method_name = STRING_c_str(queued_cb->iothub_callback.method_cb_info.method_name);
                        const unsigned char* payload = BUFFER_u_char(queued_cb->iothub_callback.method_cb_info.payload);
                        size_t payload_len = BUFFER_length(queued_cb->iothub_callback.method_cb_info.payload);

                        iotHubClientInstance->inbound_device_method_callback(method_name, payload, payload_len, queued_cb->iothub_callback.method_cb_info.method_id, queued_cb->userContextCallback);

                        BUFFER_delete(queued_cb->iothub_callback.method_cb_info.payload);
                        STRING_delete(queued_cb->iothub_callback.method_cb_info.method_name);
                    }
                    break;
                case CALLBACK_TYPE_MESSAGE:
                    if (iotHubClientInstance->message_callback)
                    {
                        IOTHUBMESSAGE_DISPOSITION_RESULT disposition = iotHubClientInstance->message_callback(queued_cb->iothub_callback.message_cb_info->messageHandle, queued_cb->userContextCallback);
                        IOTHUB_CLIENT_HANDLE handle = iotHubClientInstance->message_user_context->iotHubClientHandle;

                        if (Lock(handle->LockHandle) == LOCK_OK)
                        {
                            IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendMessageDisposition(handle->IoTHubClientLLHandle, queued_cb->iothub_callback.message_cb_info, disposition);
                            (void)Unlock(handle->LockHandle);
                            if (result != IOTHUB_CLIENT_OK)
                            {
                                LogError("IoTHubClient_LL_SendMessageDisposition failed");
                            }
                        }
                        else
                        {
                            LogError("Lock failed");
                        }
                    }
                    break;
                default:
                    LogError("Invalid callback type '%s'", ENUM_TO_STRING(USER_CALLBACK_TYPE, queued_cb->type));
                    break;
            }
        }
    }
    VECTOR_destroy(call_backs);
}

static void ScheduleWork_Thread_ForMultiplexing(void* iotHubClientHandle)
{
    IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

#ifdef USE_UPLOADTOBLOB
    garbageCollectorImpl(iotHubClientInstance);
#endif
    if (Lock(iotHubClientInstance->LockHandle) == LOCK_OK)
    {
        VECTOR_HANDLE call_backs = VECTOR_move(iotHubClientInstance->saved_user_callback_list);
        (void)Unlock(iotHubClientInstance->LockHandle);

        if (call_backs == NULL)
        {
            LogError("Failed moving user callbacks");
        }
        else
        {
            dispatch_user_callbacks(iotHubClientInstance, call_backs);
        }
    }
    else
    {
        LogError("failed locking for ScheduleWork_Thread_ForMultiplexing");
    }
}

static int ScheduleWork_Thread(void* threadArgument)
{
    IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)threadArgument;

    while (1)
    {
        if (Lock(iotHubClientInstance->LockHandle) == LOCK_OK)
        {
            /*Codes_SRS_IOTHUBCLIENT_01_038: [ The thread shall exit when IoTHubClient_Destroy is called. ]*/
            if (iotHubClientInstance->StopThread)
            {
                (void)Unlock(iotHubClientInstance->LockHandle);
                break; /*gets out of the thread*/
            }
            else
            {
                /* Codes_SRS_IOTHUBCLIENT_01_037: [The thread created by IoTHubClient_SendEvent or IoTHubClient_SetMessageCallback shall call IoTHubClient_LL_DoWork every 1 ms.] */
                /* Codes_SRS_IOTHUBCLIENT_01_039: [All calls to IoTHubClient_LL_DoWork shall be protected by the lock created in IotHubClient_Create.] */
                IoTHubClient_LL_DoWork(iotHubClientInstance->IoTHubClientLLHandle);

#ifdef USE_UPLOADTOBLOB
                garbageCollectorImpl(iotHubClientInstance);
#endif
                VECTOR_HANDLE call_backs = VECTOR_move(iotHubClientInstance->saved_user_callback_list);
                (void)Unlock(iotHubClientInstance->LockHandle);
                if (call_backs == NULL)
                {
                    LogError("VECTOR_move failed");
                }
                else
                {
                    dispatch_user_callbacks(iotHubClientInstance, call_backs);
                }
            }
        }
        else
        {
            /*Codes_SRS_IOTHUBCLIENT_01_040: [If acquiring the lock fails, IoTHubClient_LL_DoWork shall not be called.]*/
            /*no code, shall retry*/
        }
        (void)ThreadAPI_Sleep(1);
    }

    ThreadAPI_Exit(0);
    return 0;
}

static IOTHUB_CLIENT_RESULT StartWorkerThreadIfNeeded(IOTHUB_CLIENT_INSTANCE* iotHubClientInstance)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientInstance->TransportHandle == NULL)
    {
        if (iotHubClientInstance->ThreadHandle == NULL)
        {
            iotHubClientInstance->StopThread = 0;
            if (ThreadAPI_Create(&iotHubClientInstance->ThreadHandle, ScheduleWork_Thread, iotHubClientInstance) != THREADAPI_OK)
            {
                LogError("ThreadAPI_Create failed");
                iotHubClientInstance->ThreadHandle = NULL;
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }
    }
    else
    {
        /*Codes_SRS_IOTHUBCLIENT_17_012: [ If the transport connection is shared, the thread shall be started by calling IoTHubTransport_StartWorkerThread. ]*/
        /*Codes_SRS_IOTHUBCLIENT_17_011: [ If the transport connection is shared, the thread shall be started by calling IoTHubTransport_StartWorkerThread*/
        result = IoTHubTransport_StartWorkerThread(iotHubClientInstance->TransportHandle, iotHubClientInstance, ScheduleWork_Thread_ForMultiplexing);
    }
    return result;
}

static IOTHUB_CLIENT_INSTANCE* create_iothub_instance(const IOTHUB_CLIENT_CONFIG* config, TRANSPORT_HANDLE transportHandle, const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_INSTANCE* result = (IOTHUB_CLIENT_INSTANCE*)malloc(sizeof(IOTHUB_CLIENT_INSTANCE));

    /* Codes_SRS_IOTHUBCLIENT_01_004: [If allocating memory for the new IoTHubClient instance fails, then IoTHubClient_Create shall return NULL.] */
    if (result != NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_01_029: [IoTHubClient_Create shall create a lock object to be used later for serializing IoTHubClient calls.] */
        if ( (result->saved_user_callback_list = VECTOR_create(sizeof(USER_CALLBACK_INFO)) ) == NULL)
        {
            LogError("Failed creating VECTOR");
            free(result);
            result = NULL;
        }
        else
        {
#ifdef USE_UPLOADTOBLOB
            /*Codes_SRS_IOTHUBCLIENT_02_060: [ IoTHubClient_Create shall create a SINGLYLINKEDLIST_HANDLE containing THREAD_HANDLE (created by future calls to IoTHubClient_UploadToBlobAsync). ]*/
            if ((result->savedDataToBeCleaned = singlylinkedlist_create()) == NULL)
            {
                /*Codes_SRS_IOTHUBCLIENT_02_061: [ If creating the SINGLYLINKEDLIST_HANDLE fails then IoTHubClient_Create shall fail and return NULL. ]*/
                LogError("unable to singlylinkedlist_create");
                VECTOR_destroy(result->saved_user_callback_list);
                free(result);
                result = NULL;
            }
            else
#endif
            {
                result->TransportHandle = transportHandle;
                result->created_with_transport_handle = 0;
                if (config != NULL)
                {
                    if (transportHandle != NULL)
                    {
                        /*Codes_SRS_IOTHUBCLIENT_17_005: [ IoTHubClient_CreateWithTransport shall call IoTHubTransport_GetLock to get the transport lock to be used later for serializing IoTHubClient calls. ]*/
                        result->LockHandle = IoTHubTransport_GetLock(transportHandle);
                        if (result->LockHandle == NULL)
                        {
                            LogError("unable to IoTHubTransport_GetLock");
                            result->IoTHubClientLLHandle = NULL;
                        }
                        else
                        {
                            IOTHUB_CLIENT_DEVICE_CONFIG deviceConfig;
                            deviceConfig.deviceId = config->deviceId;
                            deviceConfig.deviceKey = config->deviceKey;
                            deviceConfig.protocol = config->protocol;
                            deviceConfig.deviceSasToken = config->deviceSasToken;

                            /*Codes_SRS_IOTHUBCLIENT_17_003: [ IoTHubClient_CreateWithTransport shall call IoTHubTransport_GetLLTransport on transportHandle to get lower layer transport. ]*/
                            deviceConfig.transportHandle = IoTHubTransport_GetLLTransport(transportHandle);
                            if (deviceConfig.transportHandle == NULL)
                            {
                                LogError("unable to IoTHubTransport_GetLLTransport");
                                result->IoTHubClientLLHandle = NULL;
                            }
                            else
                            {
                                if (Lock(result->LockHandle) != LOCK_OK)
                                {
                                    LogError("unable to Lock");
                                    result->IoTHubClientLLHandle = NULL;
                                }
                                else
                                {
                                    /*Codes_SRS_IOTHUBCLIENT_17_007: [ IoTHubClient_CreateWithTransport shall instantiate a new IoTHubClient_LL instance by calling IoTHubClient_LL_CreateWithTransport and passing the lower layer transport and config argument. ]*/
                                    result->IoTHubClientLLHandle = IoTHubClient_LL_CreateWithTransport(&deviceConfig);
                                    result->created_with_transport_handle = 1;
                                    if (Unlock(result->LockHandle) != LOCK_OK)
                                    {
                                        LogError("unable to Unlock");
                                        result->IoTHubClientLLHandle = NULL;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        result->LockHandle = Lock_Init();
                        if (result->LockHandle == NULL)
                        {
                            /* Codes_SRS_IOTHUBCLIENT_01_030: [If creating the lock fails, then IoTHubClient_Create shall return NULL.] */
                            /* Codes_SRS_IOTHUBCLIENT_01_031: [If IoTHubClient_Create fails, all resources allocated by it shall be freed.] */
                            LogError("Failure creating Lock object");
                            result->IoTHubClientLLHandle = NULL;
                        }
                        else 
                        {
                            /* Codes_SRS_IOTHUBCLIENT_01_002: [IoTHubClient_Create shall instantiate a new IoTHubClient_LL instance by calling IoTHubClient_LL_Create and passing the config argument.] */
                            result->IoTHubClientLLHandle = IoTHubClient_LL_Create(config);
                        }
                    }
                }
                else
                {
                    result->LockHandle = Lock_Init();
                    if (result->LockHandle == NULL)
                    {
                        /* Codes_SRS_IOTHUBCLIENT_01_030: [If creating the lock fails, then IoTHubClient_Create shall return NULL.] */
                        /* Codes_SRS_IOTHUBCLIENT_01_031: [If IoTHubClient_Create fails, all resources allocated by it shall be freed.] */
                        LogError("Failure creating Lock object");
                        result->IoTHubClientLLHandle = NULL;
                    }
                    else 
                    {
                        /* Codes_SRS_IOTHUBCLIENT_12_006: [IoTHubClient_CreateFromConnectionString shall instantiate a new IoTHubClient_LL instance by calling IoTHubClient_LL_CreateFromConnectionString and passing the connectionString] */
                        result->IoTHubClientLLHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, protocol);
                    }
                }

                if (result->IoTHubClientLLHandle == NULL)
                {
                    /* Codes_SRS_IOTHUBCLIENT_01_003: [If IoTHubClient_LL_Create fails, then IoTHubClient_Create shall return NULL.] */
                    /* Codes_SRS_IOTHUBCLIENT_01_031: [If IoTHubClient_Create fails, all resources allocated by it shall be freed.] */
                    /* Codes_SRS_IOTHUBCLIENT_17_006: [ If IoTHubTransport_GetLock fails, then IoTHubClient_CreateWithTransport shall return NULL. ]*/
                    if (transportHandle == NULL)
                    {
                        Lock_Deinit(result->LockHandle);
                    }
#ifdef USE_UPLOADTOBLOB
                    singlylinkedlist_destroy(result->savedDataToBeCleaned);
#endif
                    LogError("Failure creating iothub handle");
                    VECTOR_destroy(result->saved_user_callback_list);
                    free(result);
                    result = NULL;
                }
                else
                {
                    result->ThreadHandle = NULL;
                    result->desired_state_callback = NULL;
                    result->event_confirm_callback = NULL;
                    result->reported_state_callback = NULL;
                    result->devicetwin_user_context = NULL;
                    result->connection_status_callback = NULL;
                    result->connection_status_user_context = NULL;
                    result->message_callback = NULL;
                    result->message_user_context = NULL;
                    result->method_user_context = NULL;
                }
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_HANDLE IoTHubClient_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_INSTANCE* result;

    /* Codes_SRS_IOTHUBCLIENT_12_003: [IoTHubClient_CreateFromConnectionString shall verify all input parameters and if any is NULL then return NULL] */
    if (connectionString == NULL)
    {
        LogError("Input parameter is NULL: connectionString");
        result = NULL;
    }
    else if (protocol == NULL)
    {
        LogError("Input parameter is NULL: protocol");
        result = NULL;
    }
    else
    {
        result = create_iothub_instance(NULL, NULL, connectionString, protocol);
    }
    return result;
}

IOTHUB_CLIENT_HANDLE IoTHubClient_Create(const IOTHUB_CLIENT_CONFIG* config)
{
    IOTHUB_CLIENT_INSTANCE* result;
    if (config == NULL)
    {
        LogError("Input parameter is NULL: IOTHUB_CLIENT_CONFIG");
        result = NULL;
    }
    else
    {
        result = create_iothub_instance(config, NULL, NULL, NULL);
    }
    return result;
}

IOTHUB_CLIENT_HANDLE IoTHubClient_CreateWithTransport(TRANSPORT_HANDLE transportHandle, const IOTHUB_CLIENT_CONFIG* config)
{
    IOTHUB_CLIENT_INSTANCE* result;
    /*Codes_SRS_IOTHUBCLIENT_17_013: [ IoTHubClient_CreateWithTransport shall return NULL if transportHandle is NULL. ]*/
    /*Codes_SRS_IOTHUBCLIENT_17_014: [ IoTHubClient_CreateWithTransport shall return NULL if config is NULL. ]*/
    if (transportHandle == NULL || config == NULL)
    {
        LogError("invalid parameter TRANSPORT_HANDLE transportHandle=%p, const IOTHUB_CLIENT_CONFIG* config=%p", transportHandle, config);
        result = NULL;
    }
    else
    {
        result = create_iothub_instance(config, transportHandle, NULL, NULL);
    }
    return result;
}

/* Codes_SRS_IOTHUBCLIENT_01_005: [IoTHubClient_Destroy shall free all resources associated with the iotHubClientHandle instance.] */
void IoTHubClient_Destroy(IOTHUB_CLIENT_HANDLE iotHubClientHandle)
{
    /* Codes_SRS_IOTHUBCLIENT_01_008: [IoTHubClient_Destroy shall do nothing if parameter iotHubClientHandle is NULL.] */
    if (iotHubClientHandle != NULL)
    {
        bool okToJoin;
        size_t vector_size;

        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        if (iotHubClientInstance->TransportHandle != NULL)
        {
            /*Codes_SRS_IOTHUBCLIENT_01_007: [ The thread created as part of executing IoTHubClient_SendEventAsync or IoTHubClient_SetNotificationMessageCallback shall be joined. ]*/
            okToJoin = IoTHubTransport_SignalEndWorkerThread(iotHubClientInstance->TransportHandle, iotHubClientHandle);
        }

        /*Codes_SRS_IOTHUBCLIENT_02_043: [ IoTHubClient_Destroy shall lock the serializing lock and signal the worker thread (if any) to end ]*/
        if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
        {
            LogError("unable to Lock - - will still proceed to try to end the thread without locking");
        }

        if (iotHubClientInstance->ThreadHandle != NULL)
        {
            iotHubClientInstance->StopThread = 1;
            okToJoin = true;
        }
        else
        {
            okToJoin = false;
        }

        /*Codes_SRS_IOTHUBCLIENT_02_045: [ IoTHubClient_Destroy shall unlock the serializing lock. ]*/
        if (Unlock(iotHubClientInstance->LockHandle) != LOCK_OK)
        {
            LogError("unable to Unlock");
        }

        if (okToJoin == true)
        {
            if (iotHubClientInstance->ThreadHandle != NULL)
            {
                int res;
                /*Codes_SRS_IOTHUBCLIENT_01_007: [ The thread created as part of executing IoTHubClient_SendEventAsync or IoTHubClient_SetNotificationMessageCallback shall be joined. ]*/
                if (ThreadAPI_Join(iotHubClientInstance->ThreadHandle, &res) != THREADAPI_OK)
                {
                    LogError("ThreadAPI_Join failed");
                }
            }

            if (iotHubClientInstance->TransportHandle != NULL)
            {
                /*Codes_SRS_IOTHUBCLIENT_01_007: [ The thread created as part of executing IoTHubClient_SendEventAsync or IoTHubClient_SetNotificationMessageCallback shall be joined. ]*/
                IoTHubTransport_JoinWorkerThread(iotHubClientInstance->TransportHandle, iotHubClientHandle);
            }
        }


        if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
        {
            LogError("unable to Lock - - will still proceed to try to end the thread without locking");
        }

#ifdef USE_UPLOADTOBLOB
        /*Codes_SRS_IOTHUBCLIENT_02_069: [ IoTHubClient_Destroy shall free all data created by IoTHubClient_UploadToBlobAsync ]*/
        /*wait for all uploading threads to finish*/
        while (singlylinkedlist_get_head_item(iotHubClientInstance->savedDataToBeCleaned) != NULL)
        {
            garbageCollectorImpl(iotHubClientInstance);
        }

        if (iotHubClientInstance->savedDataToBeCleaned != NULL)
        {
            singlylinkedlist_destroy(iotHubClientInstance->savedDataToBeCleaned);
        }
#endif

        /* Codes_SRS_IOTHUBCLIENT_01_006: [That includes destroying the IoTHubClient_LL instance by calling IoTHubClient_LL_Destroy.] */
        IoTHubClient_LL_Destroy(iotHubClientInstance->IoTHubClientLLHandle);

        if (Unlock(iotHubClientInstance->LockHandle) != LOCK_OK)
        {
            LogError("unable to Unlock");
        }


        vector_size = VECTOR_size(iotHubClientInstance->saved_user_callback_list);
        size_t index = 0;
        for (index = 0; index < vector_size; index++)
        {
            USER_CALLBACK_INFO* queue_cb_info = (USER_CALLBACK_INFO*)VECTOR_element(iotHubClientInstance->saved_user_callback_list, index);
            if (queue_cb_info != NULL)
            {
                if ((queue_cb_info->type == CALLBACK_TYPE_DEVICE_METHOD) || (queue_cb_info->type == CALLBACK_TYPE_INBOUD_DEVICE_METHOD))
                {
                    STRING_delete(queue_cb_info->iothub_callback.method_cb_info.method_name);
                    BUFFER_delete(queue_cb_info->iothub_callback.method_cb_info.payload);
                }
                else if (queue_cb_info->type == CALLBACK_TYPE_DEVICE_TWIN)
                {
                    if (queue_cb_info->iothub_callback.dev_twin_cb_info.payLoad != NULL)
                    {
                        free(queue_cb_info->iothub_callback.dev_twin_cb_info.payLoad);
                    }
                }
                else if (queue_cb_info->type == CALLBACK_TYPE_EVENT_CONFIRM)
                {
                    if (iotHubClientInstance->event_confirm_callback)
                    {
                        iotHubClientInstance->event_confirm_callback(queue_cb_info->iothub_callback.event_confirm_cb_info.confirm_result, queue_cb_info->userContextCallback);
                    }
                }
            }
        }
        VECTOR_destroy(iotHubClientInstance->saved_user_callback_list);

        if (iotHubClientInstance->TransportHandle == NULL)
        {
            /* Codes_SRS_IOTHUBCLIENT_01_032: [If the lock was allocated in IoTHubClient_Create, it shall be also freed..] */
            Lock_Deinit(iotHubClientInstance->LockHandle);
        }
        if (iotHubClientInstance->devicetwin_user_context != NULL)
        {
            free(iotHubClientInstance->devicetwin_user_context);
        }
        if (iotHubClientInstance->connection_status_user_context != NULL)
        {
            free(iotHubClientInstance->connection_status_user_context);
        }
        if (iotHubClientInstance->message_user_context != NULL)
        {
            free(iotHubClientInstance->message_user_context);
        }
        if (iotHubClientInstance->method_user_context != NULL)
        {
            free(iotHubClientInstance->method_user_context);
        }
        free(iotHubClientInstance);
    }
}

IOTHUB_CLIENT_RESULT IoTHubClient_SendEventAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_01_011: [If iotHubClientHandle is NULL, IoTHubClient_SendEventAsync shall return IOTHUB_CLIENT_INVALID_ARG.] */
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("NULL iothubClientHandle");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /* Codes_SRS_IOTHUBCLIENT_01_009: [IoTHubClient_SendEventAsync shall start the worker thread if it was not previously started.] */
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            /* Codes_SRS_IOTHUBCLIENT_01_010: [If starting the thread fails, IoTHubClient_SendEventAsync shall return IOTHUB_CLIENT_ERROR.] */
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            if (iotHubClientInstance->created_with_transport_handle == 0)
            {
                iotHubClientInstance->event_confirm_callback = eventConfirmationCallback;
            }

            /* Codes_SRS_IOTHUBCLIENT_01_025: [IoTHubClient_SendEventAsync shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                /* Codes_SRS_IOTHUBCLIENT_01_026: [If acquiring the lock fails, IoTHubClient_SendEventAsync shall return IOTHUB_CLIENT_ERROR.] */
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                if (iotHubClientInstance->created_with_transport_handle != 0 || eventConfirmationCallback == NULL)
                {
                    result = IoTHubClient_LL_SendEventAsync(iotHubClientInstance->IoTHubClientLLHandle, eventMessageHandle, eventConfirmationCallback, userContextCallback);
                }
                else
                {
                    /* Codes_SRS_IOTHUBCLIENT_07_001: [ IoTHubClient_SendEventAsync shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClient_LL_SendEventAsync function as a user context. ] */
                    IOTHUB_QUEUE_CONTEXT* queue_context = (IOTHUB_QUEUE_CONTEXT*)malloc(sizeof(IOTHUB_QUEUE_CONTEXT));
                    if (queue_context == NULL)
                    {
                        result = IOTHUB_CLIENT_ERROR;
                        LogError("Failed allocating QUEUE_CONTEXT");
                    }
                    else
                    {
                        queue_context->iotHubClientHandle = iotHubClientInstance;
                        queue_context->userContextCallback = userContextCallback;
                        /* Codes_SRS_IOTHUBCLIENT_01_012: [IoTHubClient_SendEventAsync shall call IoTHubClient_LL_SendEventAsync, while passing the IoTHubClient_LL handle created by IoTHubClient_Create and the parameters eventMessageHandle, eventConfirmationCallback and userContextCallback.] */
                        /* Codes_SRS_IOTHUBCLIENT_01_013: [When IoTHubClient_LL_SendEventAsync is called, IoTHubClient_SendEventAsync shall return the result of IoTHubClient_LL_SendEventAsync.] */
                        result = IoTHubClient_LL_SendEventAsync(iotHubClientInstance->IoTHubClientLLHandle, eventMessageHandle, iothub_ll_event_confirm_callback, queue_context);
                        if (result != IOTHUB_CLIENT_OK)
                        {
                            LogError("IoTHubClient_LL_SendEventAsync failed");
                            free(queue_context);
                        }
                    }
                }

                /* Codes_SRS_IOTHUBCLIENT_01_025: [IoTHubClient_SendEventAsync shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
                (void)Unlock(iotHubClientInstance->LockHandle);
            }
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_GetSendStatus(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_01_023: [If iotHubClientHandle is NULL, IoTHubClient_ GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG.] */
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("NULL iothubClientHandle");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /* Codes_SRS_IOTHUBCLIENT_01_033: [IoTHubClient_GetSendStatus shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
        if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
        {
            /* Codes_SRS_IOTHUBCLIENT_01_034: [If acquiring the lock fails, IoTHubClient_GetSendStatus shall return IOTHUB_CLIENT_ERROR.] */
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not acquire lock");
        }
        else
        {
            /* Codes_SRS_IOTHUBCLIENT_01_022: [IoTHubClient_GetSendStatus shall call IoTHubClient_LL_GetSendStatus, while passing the IoTHubClient_LL handle created by IoTHubClient_Create and the parameter iotHubClientStatus.] */
            /* Codes_SRS_IOTHUBCLIENT_01_024: [Otherwise, IoTHubClient_GetSendStatus shall return the result of IoTHubClient_LL_GetSendStatus.] */
            result = IoTHubClient_LL_GetSendStatus(iotHubClientInstance->IoTHubClientLLHandle, iotHubClientStatus);

            /* Codes_SRS_IOTHUBCLIENT_01_033: [IoTHubClient_GetSendStatus shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
            (void)Unlock(iotHubClientInstance->LockHandle);
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_SetMessageCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_01_016: [If iotHubClientHandle is NULL, IoTHubClient_SetMessageCallback shall return IOTHUB_CLIENT_INVALID_ARG.] */
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("NULL iothubClientHandle");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /* Codes_SRS_IOTHUBCLIENT_01_014: [IoTHubClient_SetMessageCallback shall start the worker thread if it was not previously started.] */
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            /* Codes_SRS_IOTHUBCLIENT_01_015: [If starting the thread fails, IoTHubClient_SetMessageCallback shall return IOTHUB_CLIENT_ERROR.] */
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            if (iotHubClientInstance->created_with_transport_handle == 0)
            {
                iotHubClientInstance->message_callback = messageCallback;
            }

            /* Codes_SRS_IOTHUBCLIENT_01_027: [IoTHubClient_SetMessageCallback shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                /* Codes_SRS_IOTHUBCLIENT_01_028: [If acquiring the lock fails, IoTHubClient_SetMessageCallback shall return IOTHUB_CLIENT_ERROR.] */
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                if (iotHubClientInstance->message_user_context != NULL)
                {
                    free(iotHubClientInstance->message_user_context);
                    iotHubClientInstance->message_user_context = NULL;
                }
                if (messageCallback == NULL)
                {
                    result = IoTHubClient_LL_SetMessageCallback_Ex(iotHubClientInstance->IoTHubClientLLHandle, NULL, iotHubClientInstance->message_user_context);
                }
                else if (iotHubClientInstance->created_with_transport_handle != 0)
                {
                    result = IoTHubClient_LL_SetMessageCallback(iotHubClientInstance->IoTHubClientLLHandle, messageCallback, userContextCallback);
                }
                else
                {
                    iotHubClientInstance->message_user_context = (IOTHUB_QUEUE_CONTEXT*)malloc(sizeof(IOTHUB_QUEUE_CONTEXT));
                    if (iotHubClientInstance->message_user_context == NULL)
                    {
                        result = IOTHUB_CLIENT_ERROR;
                        LogError("Failed allocating QUEUE_CONTEXT");
                    }
                    else
                    {
                        iotHubClientInstance->message_user_context->iotHubClientHandle = iotHubClientHandle;
                        iotHubClientInstance->message_user_context->userContextCallback = userContextCallback;

                        /* Codes_SRS_IOTHUBCLIENT_01_017: [IoTHubClient_SetMessageCallback shall call IoTHubClient_LL_SetMessageCallback_Ex, while passing the IoTHubClient_LL handle created by IoTHubClient_Create and the local iothub_ll_message_callback wrapper of messageCallback and userContextCallback.] */
                        /* Codes_SRS_IOTHUBCLIENT_01_018: [When IoTHubClient_LL_SetMessageCallback_Ex is called, IoTHubClient_SetMessageCallback shall return the result of IoTHubClient_LL_SetMessageCallback_Ex.] */
                        result = IoTHubClient_LL_SetMessageCallback_Ex(iotHubClientInstance->IoTHubClientLLHandle, iothub_ll_message_callback, iotHubClientInstance->message_user_context);
                        if (result != IOTHUB_CLIENT_OK)
                        {
                            LogError("IoTHubClient_LL_SetMessageCallback failed");
                            free(iotHubClientInstance->message_user_context);
                            iotHubClientInstance->message_user_context = NULL;
                        }
                    }
                }

                /* Codes_SRS_IOTHUBCLIENT_01_027: [IoTHubClient_SetMessageCallback shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
                (void)Unlock(iotHubClientInstance->LockHandle);
            }
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_SetConnectionStatusCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void * userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_25_076: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetRetryPolicy` shall return `IOTHUB_CLIENT_INVALID_ARG`. ] */
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("NULL iothubClientHandle");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /* Codes_SRS_IOTHUBCLIENT_25_081: [ `IoTHubClient_SetConnectionStatusCallback` shall start the worker thread if it was not previously started. ]*/
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            /* Codes_SRS_IOTHUBCLIENT_25_083: [ If starting the thread fails, `IoTHubClient_SetConnectionStatusCallback` shall return `IOTHUB_CLIENT_ERROR`. ]*/
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            /* Codes_SRS_IOTHUBCLIENT_25_087: [ `IoTHubClient_SetConnectionStatusCallback` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. ] */
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                /* Codes_SRS_IOTHUBCLIENT_25_088: [ If acquiring the lock fails, `IoTHubClient_SetConnectionStatusCallback` shall return `IOTHUB_CLIENT_ERROR`. ]*/
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                if (iotHubClientInstance->created_with_transport_handle == 0)
                {
                    iotHubClientInstance->connection_status_callback = connectionStatusCallback;
                }

                if (iotHubClientInstance->created_with_transport_handle != 0 || connectionStatusCallback == NULL)
                {
                    /* Codes_SRS_IOTHUBCLIENT_25_085: [ `IoTHubClient_SetConnectionStatusCallback` shall call `IoTHubClient_LL_SetConnectionStatusCallback`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the parameters `connectionStatusCallback` and `userContextCallback`. ]*/
                    result = IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientInstance->IoTHubClientLLHandle, connectionStatusCallback, userContextCallback);
                }
                else
                {
                    if (iotHubClientInstance->connection_status_user_context != NULL)
                    {
                        free(iotHubClientInstance->connection_status_user_context);
                    }
                    iotHubClientInstance->connection_status_user_context = (IOTHUB_QUEUE_CONTEXT*)malloc(sizeof(IOTHUB_QUEUE_CONTEXT));
                    if (iotHubClientInstance->connection_status_user_context == NULL)
                    {
                        result = IOTHUB_CLIENT_ERROR;
                        LogError("Failed allocating QUEUE_CONTEXT");
                    }
                    else
                    {
                        iotHubClientInstance->connection_status_user_context->iotHubClientHandle = iotHubClientInstance;
                        iotHubClientInstance->connection_status_user_context->userContextCallback = userContextCallback;

                        /* Codes_SRS_IOTHUBCLIENT_25_085: [ `IoTHubClient_SetConnectionStatusCallback` shall call `IoTHubClient_LL_SetConnectionStatusCallback`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the parameters `connectionStatusCallback` and `userContextCallback`. ]*/
                        result = IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientInstance->IoTHubClientLLHandle, iothub_ll_connection_status_callback, iotHubClientInstance->connection_status_user_context);
                        if (result != IOTHUB_CLIENT_OK)
                        {
                            LogError("IoTHubClient_LL_SetConnectionStatusCallback failed");
                            free(iotHubClientInstance->connection_status_user_context);
                            iotHubClientInstance->connection_status_user_context = NULL;
                        }
                    }
                }
                (void)Unlock(iotHubClientInstance->LockHandle);
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_SetRetryPolicy(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_25_076: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SetRetryPolicy` shall return `IOTHUB_CLIENT_INVALID_ARG`. ] */
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("NULL iothubClientHandle");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /* Codes_SRS_IOTHUBCLIENT_25_073: [ `IoTHubClient_SetRetryPolicy` shall start the worker thread if it was not previously started. ] */
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            /* Codes_SRS_IOTHUBCLIENT_25_075: [ If starting the thread fails, `IoTHubClient_SetRetryPolicy` shall return `IOTHUB_CLIENT_ERROR`. ]*/
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            /* Codes_SRS_IOTHUBCLIENT_25_079: [ `IoTHubClient_SetRetryPolicy` shall be made thread-safe by using the lock created in `IoTHubClient_Create`.] */
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                /* Codes_SRS_IOTHUBCLIENT_25_080: [ If acquiring the lock fails, `IoTHubClient_SetRetryPolicy` shall return `IOTHUB_CLIENT_ERROR`. ]*/
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                /* Codes_SRS_IOTHUBCLIENT_25_077: [ `IoTHubClient_SetRetryPolicy` shall call `IoTHubClient_LL_SetRetryPolicy`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the parameters `retryPolicy` and `retryTimeoutLimitinSeconds`.]*/
                result = IoTHubClient_LL_SetRetryPolicy(iotHubClientInstance->IoTHubClientLLHandle, retryPolicy, retryTimeoutLimitInSeconds);
                (void)Unlock(iotHubClientInstance->LockHandle);
            }

        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_GetRetryPolicy(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimitInSeconds)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_25_092: [ If `iotHubClientHandle` is `NULL`, `IoTHubClient_GetRetryPolicy` shall return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("NULL iothubClientHandle");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /* Codes_SRS_IOTHUBCLIENT_25_089: [ `IoTHubClient_GetRetryPolicy` shall start the worker thread if it was not previously started.]*/
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            /* Codes_SRS_IOTHUBCLIENT_25_091: [ If starting the thread fails, `IoTHubClient_GetRetryPolicy` shall return `IOTHUB_CLIENT_ERROR`.]*/
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            /* Codes_SRS_IOTHUBCLIENT_25_095: [ `IoTHubClient_GetRetryPolicy` shall be made thread-safe by using the lock created in `IoTHubClient_Create`. ]*/
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                /* Codes_SRS_IOTHUBCLIENT_25_096: [ If acquiring the lock fails, `IoTHubClient_GetRetryPolicy` shall return `IOTHUB_CLIENT_ERROR`. ]*/
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                /* Codes_SRS_IOTHUBCLIENT_25_093: [ `IoTHubClient_GetRetryPolicy` shall call `IoTHubClient_LL_GetRetryPolicy`, while passing the `IoTHubClient_LL` handle created by `IoTHubClient_Create` and the parameters `connectionStatusCallback` and `userContextCallback`.]*/
                result = IoTHubClient_LL_GetRetryPolicy(iotHubClientInstance->IoTHubClientLLHandle, retryPolicy, retryTimeoutLimitInSeconds);
                (void)Unlock(iotHubClientInstance->LockHandle);
            }
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_GetLastMessageReceiveTime(IOTHUB_CLIENT_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime)
{
    IOTHUB_CLIENT_RESULT result;

    if (iotHubClientHandle == NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_01_020: [If iotHubClientHandle is NULL, IoTHubClient_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_INVALID_ARG.] */
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("NULL iothubClientHandle");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /* Codes_SRS_IOTHUBCLIENT_01_035: [IoTHubClient_GetLastMessageReceiveTime shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
        if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
        {
            /* Codes_SRS_IOTHUBCLIENT_01_036: [If acquiring the lock fails, IoTHubClient_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_ERROR.] */
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not acquire lock");
        }
        else
        {
            /* Codes_SRS_IOTHUBCLIENT_01_019: [IoTHubClient_GetLastMessageReceiveTime shall call IoTHubClient_LL_GetLastMessageReceiveTime, while passing the IoTHubClient_LL handle created by IoTHubClient_Create and the parameter lastMessageReceiveTime.] */
            /* Codes_SRS_IOTHUBCLIENT_01_021: [Otherwise, IoTHubClient_GetLastMessageReceiveTime shall return the result of IoTHubClient_LL_GetLastMessageReceiveTime.] */
            result = IoTHubClient_LL_GetLastMessageReceiveTime(iotHubClientInstance->IoTHubClientLLHandle, lastMessageReceiveTime);

            /* Codes_SRS_IOTHUBCLIENT_01_035: [IoTHubClient_GetLastMessageReceiveTime shall be made thread-safe by using the lock created in IoTHubClient_Create.] */
            (void)Unlock(iotHubClientInstance->LockHandle);
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_SetOption(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* optionName, const void* value)
{
    IOTHUB_CLIENT_RESULT result;
    /*Codes_SRS_IOTHUBCLIENT_02_034: [If parameter iotHubClientHandle is NULL then IoTHubClient_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
    /*Codes_SRS_IOTHUBCLIENT_02_035: [ If parameter optionName is NULL then IoTHubClient_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
    /*Codes_SRS_IOTHUBCLIENT_02_036: [ If parameter value is NULL then IoTHubClient_SetOption shall return IOTHUB_CLIENT_INVALID_ARG. ]*/
    if (
        (iotHubClientHandle == NULL) ||
        (optionName == NULL) ||
        (value == NULL)
        )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid arg (NULL)");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /* Codes_SRS_IOTHUBCLIENT_01_041: [ IoTHubClient_SetOption shall be made thread-safe by using the lock created in IoTHubClient_Create. ]*/
        if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
        {
            /* Codes_SRS_IOTHUBCLIENT_01_042: [ If acquiring the lock fails, IoTHubClient_SetOption shall return IOTHUB_CLIENT_ERROR. ]*/
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not acquire lock");
        }
        else
        {
            /*Codes_SRS_IOTHUBCLIENT_02_038: [If optionName doesn't match one of the options handled by this module then IoTHubClient_SetOption shall call IoTHubClient_LL_SetOption passing the same parameters and return what IoTHubClient_LL_SetOption returns.] */
            result = IoTHubClient_LL_SetOption(iotHubClientInstance->IoTHubClientLLHandle, optionName, value);
            if (result != IOTHUB_CLIENT_OK)
            {
                LogError("IoTHubClient_LL_SetOption failed");
            }

            (void)Unlock(iotHubClientInstance->LockHandle);
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_SetDeviceTwinCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    /*Codes_SRS_IOTHUBCLIENT_10_001: [** `IoTHubClient_SetDeviceTwinCallback` shall fail and return `IOTHUB_CLIENT_INVALID_ARG` if parameter `iotHubClientHandle` is `NULL`. ]*/
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid arg (NULL)");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /*Codes_SRS_IOTHUBCLIENT_10_003: [** If the transport connection is shared, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. ]*/
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            /*Codes_SRS_IOTHUBCLIENT_10_004: [** If starting the thread fails, `IoTHubClient_SetDeviceTwinCallback` shall return `IOTHUB_CLIENT_ERROR`. ]*/
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            /*Codes_SRS_IOTHUBCLIENT_10_020: [** `IoTHubClient_SetDeviceTwinCallback` shall be made thread - safe by using the lock created in IoTHubClient_Create. ]*/
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                /*Codes_SRS_IOTHUBCLIENT_10_002: [** If acquiring the lock fails, `IoTHubClient_SetDeviceTwinCallback` shall return `IOTHUB_CLIENT_ERROR`. ]*/
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                if (iotHubClientInstance->created_with_transport_handle == 0)
                {
                    iotHubClientInstance->desired_state_callback = deviceTwinCallback;
                }

                if (iotHubClientInstance->created_with_transport_handle != 0 || deviceTwinCallback == NULL)
                {
                    /*Codes_SRS_IOTHUBCLIENT_10_005: [** `IoTHubClient_LL_SetDeviceTwinCallback` shall call `IoTHubClient_LL_SetDeviceTwinCallback`, while passing the `IoTHubClient_LL handle` created by `IoTHubClient_LL_Create` along with the parameters `reportedStateCallback` and `userContextCallback`. ]*/
                    result = IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientInstance->IoTHubClientLLHandle, deviceTwinCallback, userContextCallback);
                }
                else
                {
                    if (iotHubClientInstance->devicetwin_user_context != NULL)
                    {
                        free(iotHubClientInstance->devicetwin_user_context);
                    }

                    /*Codes_SRS_IOTHUBCLIENT_07_002: [ IoTHubClient_SetDeviceTwinCallback shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClient_LL_SetDeviceTwinCallback function as a user context. ]*/
                    iotHubClientInstance->devicetwin_user_context = (IOTHUB_QUEUE_CONTEXT*)malloc(sizeof(IOTHUB_QUEUE_CONTEXT));
                    if (iotHubClientInstance->devicetwin_user_context == NULL)
                    {
                        result = IOTHUB_CLIENT_ERROR;
                        LogError("Failed allocating QUEUE_CONTEXT");
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBCLIENT_10_005: [** `IoTHubClient_LL_SetDeviceTwinCallback` shall call `IoTHubClient_LL_SetDeviceTwinCallback`, while passing the `IoTHubClient_LL handle` created by `IoTHubClient_LL_Create` along with the parameters `iothub_ll_device_twin_callback` and IOTHUB_QUEUE_CONTEXT variable. ]*/
                        iotHubClientInstance->devicetwin_user_context->iotHubClientHandle = iotHubClientInstance;
                        iotHubClientInstance->devicetwin_user_context->userContextCallback = userContextCallback;
                        result = IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientInstance->IoTHubClientLLHandle, iothub_ll_device_twin_callback, iotHubClientInstance->devicetwin_user_context);
                        if (result != IOTHUB_CLIENT_OK)
                        {
                            LogError("IoTHubClient_LL_SetDeviceTwinCallback failed");
                            free(iotHubClientInstance->devicetwin_user_context);
                            iotHubClientInstance->devicetwin_user_context = NULL;
                        }
                    }
                }

                (void)Unlock(iotHubClientInstance->LockHandle);
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_SendReportedState(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    /*Codes_SRS_IOTHUBCLIENT_10_013: [** If `iotHubClientHandle` is `NULL`, `IoTHubClient_SendReportedState` shall return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid arg (NULL)");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /*Codes_SRS_IOTHUBCLIENT_10_015: [** If the transport connection is shared, the thread shall be started by calling `IoTHubTransport_StartWorkerThread`. ]*/
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            /*Codes_SRS_IOTHUBCLIENT_10_016: [** If starting the thread fails, `IoTHubClient_SendReportedState` shall return `IOTHUB_CLIENT_ERROR`. ]*/
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            if (iotHubClientInstance->created_with_transport_handle == 0)
            {
                iotHubClientInstance->reported_state_callback = reportedStateCallback;
            }

            /*Codes_SRS_IOTHUBCLIENT_10_021: [** `IoTHubClient_SendReportedState` shall be made thread-safe by using the lock created in IoTHubClient_Create. ]*/
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                /*Codes_SRS_IOTHUBCLIENT_10_014: [** If acquiring the lock fails, `IoTHubClient_SendReportedState` shall return `IOTHUB_CLIENT_ERROR`. ]*/
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                if (iotHubClientInstance->created_with_transport_handle != 0 || reportedStateCallback == NULL)
                {
                    /*Codes_SRS_IOTHUBCLIENT_10_017: [** `IoTHubClient_SendReportedState` shall call `IoTHubClient_LL_SendReportedState`, while passing the `IoTHubClient_LL handle` created by `IoTHubClient_LL_Create` along with the parameters `reportedState`, `size`, `reportedStateCallback`, and `userContextCallback`. ]*/
                    /*Codes_SRS_IOTHUBCLIENT_10_018: [** When `IoTHubClient_LL_SendReportedState` is called, `IoTHubClient_SendReportedState` shall return the result of `IoTHubClient_LL_SendReportedState`. **]*/
                    result = IoTHubClient_LL_SendReportedState(iotHubClientInstance->IoTHubClientLLHandle, reportedState, size, reportedStateCallback, userContextCallback);
                }
                else
                {
                    /* Codes_SRS_IOTHUBCLIENT_07_003: [ IoTHubClient_SendReportedState shall allocate a IOTHUB_QUEUE_CONTEXT object to be sent to the IoTHubClient_LL_SendReportedState function as a user context. ] */
                    IOTHUB_QUEUE_CONTEXT* queue_context = (IOTHUB_QUEUE_CONTEXT*)malloc(sizeof(IOTHUB_QUEUE_CONTEXT));
                    if (queue_context == NULL)
                    {
                        result = IOTHUB_CLIENT_ERROR;
                        LogError("Failed allocating QUEUE_CONTEXT");
                    }
                    else
                    {
                        queue_context->iotHubClientHandle = iotHubClientInstance;
                        queue_context->userContextCallback = userContextCallback;
                        /*Codes_SRS_IOTHUBCLIENT_10_017: [** `IoTHubClient_SendReportedState` shall call `IoTHubClient_LL_SendReportedState`, while passing the `IoTHubClient_LL handle` created by `IoTHubClient_LL_Create` along with the parameters `reportedState`, `size`, `iothub_ll_reported_state_callback` and IOTHUB_QUEUE_CONTEXT variable. ]*/
                        /*Codes_SRS_IOTHUBCLIENT_10_018: [** When `IoTHubClient_LL_SendReportedState` is called, `IoTHubClient_SendReportedState` shall return the result of `IoTHubClient_LL_SendReportedState`. **]*/
                        result = IoTHubClient_LL_SendReportedState(iotHubClientInstance->IoTHubClientLLHandle, reportedState, size, iothub_ll_reported_state_callback, queue_context);
                        if (result != IOTHUB_CLIENT_OK)
                        {
                            LogError("IoTHubClient_LL_SendReportedState failed");
                            free(queue_context);
                        }
                    }
                }

                (void)Unlock(iotHubClientInstance->LockHandle);
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_SetDeviceMethodCallback(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    /*Codes_SRS_IOTHUBCLIENT_12_012: [ If iotHubClientHandle is NULL, IoTHubClient_SetDeviceMethodCallback shall return IOTHUB_CLIENT_INVALID_ARG. ]*/ 
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid arg (NULL)");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /*Codes_SRS_IOTHUBCLIENT_12_014: [ If the transport handle is null and the worker thread is not initialized, the thread shall be started by calling IoTHubTransport_StartWorkerThread. ]*/
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            /*Codes_SRS_IOTHUBCLIENT_12_015: [ If starting the thread fails, IoTHubClient_SetDeviceMethodCallback shall return IOTHUB_CLIENT_ERROR. ]*/
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            if (iotHubClientInstance->created_with_transport_handle == 0)
            {
                iotHubClientInstance->device_method_callback = deviceMethodCallback;
            }

            /*Codes_SRS_IOTHUBCLIENT_12_018: [ IoTHubClient_SetDeviceMethodCallback shall be made thread-safe by using the lock created in IoTHubClient_Create. ]*/
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                /*Codes_SRS_IOTHUBCLIENT_12_013: [ If acquiring the lock fails, IoTHubClient_SetDeviceMethodCallback shall return IOTHUB_CLIENT_ERROR. ]*/
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                if (iotHubClientInstance->method_user_context)
                {
                    free(iotHubClientInstance->method_user_context);
                    iotHubClientInstance->method_user_context = NULL;
                }
                if (deviceMethodCallback == NULL)
                {
                    result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(iotHubClientInstance->IoTHubClientLLHandle, NULL, NULL);
                }
                else
                {
                    iotHubClientInstance->method_user_context = (IOTHUB_QUEUE_CONTEXT*)malloc(sizeof(IOTHUB_QUEUE_CONTEXT));
                    if (iotHubClientInstance->method_user_context == NULL)
                    {
                        result = IOTHUB_CLIENT_ERROR;
                        LogError("Failed allocating QUEUE_CONTEXT");
                    }
                    else
                    {
                        iotHubClientInstance->method_user_context->iotHubClientHandle = iotHubClientHandle;
                        iotHubClientInstance->method_user_context->userContextCallback = userContextCallback;

                        /*Codes_SRS_IOTHUBCLIENT_12_016: [ IoTHubClient_SetDeviceMethodCallback shall call IoTHubClient_LL_SetDeviceMethodCallback, while passing the IoTHubClient_LL_handle created by IoTHubClient_LL_Create along with the parameters deviceMethodCallback and userContextCallback. ]*/
                        /*Codes_SRS_IOTHUBCLIENT_12_017: [ When IoTHubClient_LL_SetDeviceMethodCallback is called, IoTHubClient_SetDeviceMethodCallback shall return the result of IoTHubClient_LL_SetDeviceMethodCallback. ]*/
                        result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(iotHubClientInstance->IoTHubClientLLHandle, iothub_ll_device_method_callback, iotHubClientInstance->method_user_context);
                        if (result != IOTHUB_CLIENT_OK)
                        {
                            LogError("IoTHubClient_LL_SetDeviceMethodCallback_Ex failed");
                            free(iotHubClientInstance->method_user_context);
                            iotHubClientInstance->method_user_context = NULL;
                        }
                        else
                        {
                            iotHubClientInstance->device_method_callback = deviceMethodCallback;
                        }
                    }
                }

                (void)Unlock(iotHubClientInstance->LockHandle);
            }

        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    /*Codes_SRS_IOTHUBCLIENT_07_001: [ If iotHubClientHandle is NULL, IoTHubClient_SetDeviceMethodCallback_Ex shall return IOTHUB_CLIENT_INVALID_ARG. ]*/ 
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid arg (NULL)");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /*Codes_SRS_IOTHUBCLIENT_07_003: [ If the transport handle is NULL and the worker thread is not initialized, the thread shall be started by calling IoTHubTransport_StartWorkerThread. ]*/
        if ((result = StartWorkerThreadIfNeeded(iotHubClientInstance)) != IOTHUB_CLIENT_OK)
        {
            /*Codes_SRS_IOTHUBCLIENT_07_004: [ If starting the thread fails, IoTHubClient_SetDeviceMethodCallback_Ex shall return IOTHUB_CLIENT_ERROR. ]*/
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not start worker thread");
        }
        else
        {
            if (iotHubClientInstance->created_with_transport_handle == 0)
            {
                iotHubClientInstance->inbound_device_method_callback = inboundDeviceMethodCallback;
            }

            /*Codes_SRS_IOTHUBCLIENT_07_007: [ IoTHubClient_SetDeviceMethodCallback_Ex shall be made thread-safe by using the lock created in IoTHubClient_Create. ]*/
            if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
            {
                /*Codes_SRS_IOTHUBCLIENT_07_002: [ If acquiring the lock fails, IoTHubClient_SetDeviceMethodCallback_Ex shall return IOTHUB_CLIENT_ERROR. ]*/
                result = IOTHUB_CLIENT_ERROR;
                LogError("Could not acquire lock");
            }
            else
            {
                if (iotHubClientInstance->method_user_context)
                {
                    free(iotHubClientInstance->method_user_context);
                    iotHubClientInstance->method_user_context = NULL;
                }
                if (inboundDeviceMethodCallback == NULL)
                {
                    /* Codes_SRS_IOTHUBCLIENT_07_008: [ If inboundDeviceMethodCallback is NULL, IoTHubClient_SetDeviceMethodCallback_Ex shall call IoTHubClient_LL_SetDeviceMethodCallback_Ex, passing NULL for the iothub_ll_inbound_device_method_callback. ] */
                    result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(iotHubClientInstance->IoTHubClientLLHandle, NULL, NULL);
                }
                else
                {
                    iotHubClientInstance->method_user_context = (IOTHUB_QUEUE_CONTEXT*)malloc(sizeof(IOTHUB_QUEUE_CONTEXT));
                    if (iotHubClientInstance->method_user_context == NULL)
                    {
                        result = IOTHUB_CLIENT_ERROR;
                        LogError("Failed allocating QUEUE_CONTEXT");
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBCLIENT_07_005: [ IoTHubClient_SetDeviceMethodCallback_Ex shall call IoTHubClient_LL_SetDeviceMethodCallback_Ex, while passing the IoTHubClient_LL_handle created by IoTHubClient_LL_Create along with the parameters iothub_ll_inbound_device_method_callback and IOTHUB_QUEUE_CONTEXT. ]*/
                        iotHubClientInstance->method_user_context->iotHubClientHandle = iotHubClientHandle;
                        iotHubClientInstance->method_user_context->userContextCallback = userContextCallback;

                        /* Codes_SRS_IOTHUBCLIENT_07_006: [ When IoTHubClient_LL_SetDeviceMethodCallback_Ex is called, IoTHubClient_SetDeviceMethodCallback_Ex shall return the result of IoTHubClient_LL_SetDeviceMethodCallback_Ex. ] */
                        result = IoTHubClient_LL_SetDeviceMethodCallback_Ex(iotHubClientInstance->IoTHubClientLLHandle, iothub_ll_inbound_device_method_callback, iotHubClientInstance->method_user_context);
                        if (result != IOTHUB_CLIENT_OK)
                        {
                            LogError("IoTHubClient_LL_SetDeviceMethodCallback_Ex failed");
                            free(iotHubClientInstance->method_user_context);
                            iotHubClientInstance->method_user_context = NULL;
                        }
                        else
                        {
                            iotHubClientInstance->inbound_device_method_callback = inboundDeviceMethodCallback;
                        }
                    }
                }

                (void)Unlock(iotHubClientInstance->LockHandle);
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_DeviceMethodResponse(IOTHUB_CLIENT_HANDLE iotHubClientHandle, METHOD_HANDLE methodId, const unsigned char* response, size_t respSize, int statusCode)
{
    IOTHUB_CLIENT_RESULT result;

    /*Codes_SRS_IOTHUBCLIENT_12_012: [ If iotHubClientHandle is NULL, IoTHubClient_SetDeviceMethodCallback shall return IOTHUB_CLIENT_INVALID_ARG. ]*/ 
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid arg (NULL)");
    }
    else
    {
        IOTHUB_CLIENT_INSTANCE* iotHubClientInstance = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

        /*Codes_SRS_IOTHUBCLIENT_12_018: [ IoTHubClient_SetDeviceMethodCallback shall be made thread-safe by using the lock created in IoTHubClient_Create. ]*/
        if (Lock(iotHubClientInstance->LockHandle) != LOCK_OK)
        {
            /*Codes_SRS_IOTHUBCLIENT_12_013: [ If acquiring the lock fails, IoTHubClient_SetDeviceMethodCallback shall return IOTHUB_CLIENT_ERROR. ]*/
            result = IOTHUB_CLIENT_ERROR;
            LogError("Could not acquire lock");
        }
        else
        {
            result = IoTHubClient_LL_DeviceMethodResponse(iotHubClientInstance->IoTHubClientLLHandle, methodId, response, respSize, statusCode);
            if (result != IOTHUB_CLIENT_OK)
            {
                LogError("IoTHubClient_LL_DeviceMethodResponse failed");
            }
            (void)Unlock(iotHubClientInstance->LockHandle);
        }
    }
    return result;
}

#ifdef USE_UPLOADTOBLOB
static int uploadingThread(void *data)
{
    UPLOADTOBLOB_SAVED_DATA* savedData = (UPLOADTOBLOB_SAVED_DATA*)data;

    if (Lock(savedData->iotHubClientHandle->LockHandle) == LOCK_OK)
    {
        IOTHUB_CLIENT_FILE_UPLOAD_RESULT upload_result;
        /*it so happens that IoTHubClient_LL_UploadToBlob is thread-safe because there's no saved state in the handle and there are no globals, so no need to protect it*/
        /*not having it protected means multiple simultaneous uploads can happen*/
        /*Codes_SRS_IOTHUBCLIENT_02_054: [ The thread shall call IoTHubClient_LL_UploadToBlob passing the information packed in the structure. ]*/
        if (IoTHubClient_LL_UploadToBlob(savedData->iotHubClientHandle->IoTHubClientLLHandle, savedData->destinationFileName, savedData->source, savedData->size) == IOTHUB_CLIENT_OK)
        {
            upload_result = FILE_UPLOAD_OK;
        }
        else
        {
            LogError("unable to IoTHubClient_LL_UploadToBlob");
            upload_result = FILE_UPLOAD_ERROR;
        }
        (void)Unlock(savedData->iotHubClientHandle->LockHandle);

        if (savedData->iotHubClientFileUploadCallback != NULL)
        {
            /*Codes_SRS_IOTHUBCLIENT_02_055: [ If IoTHubClient_LL_UploadToBlob fails then the thread shall call iotHubClientFileUploadCallbackInternal passing as result FILE_UPLOAD_ERROR and as context the structure from SRS IOTHUBCLIENT 02 051. ]*/
            savedData->iotHubClientFileUploadCallback(upload_result, savedData->context);
        }
    }
    else
    {
        LogError("Lock failed");
    }

    /*Codes_SRS_IOTHUBCLIENT_02_071: [ The thread shall mark itself as disposable. ]*/
    if (Lock(savedData->lockGarbage) != LOCK_OK)
    {
        LogError("unable to Lock - trying anyway");
        savedData->canBeGarbageCollected = 1;
    }
    else
    {
        savedData->canBeGarbageCollected = 1;

        if (Unlock(savedData->lockGarbage) != LOCK_OK)
        {
            LogError("unable to Unlock after locking");
        }
    }

    ThreadAPI_Exit(0);
    return 0;
}
#endif

#ifdef USE_UPLOADTOBLOB
IOTHUB_CLIENT_RESULT IoTHubClient_UploadToBlobAsync(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const char* destinationFileName, const unsigned char* source, size_t size, IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK iotHubClientFileUploadCallback, void* context)
{
    IOTHUB_CLIENT_RESULT result;
    /*Codes_SRS_IOTHUBCLIENT_02_047: [ If iotHubClientHandle is NULL then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    /*Codes_SRS_IOTHUBCLIENT_02_048: [ If destinationFileName is NULL then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    /*Codes_SRS_IOTHUBCLIENT_02_049: [ If source is NULL and size is greated than 0 then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    if (
        (iotHubClientHandle == NULL) ||
        (destinationFileName == NULL) ||
        ((source == NULL) && (size > 0))
        )
    {
        LogError("invalid parameters IOTHUB_CLIENT_HANDLE iotHubClientHandle = %p , const char* destinationFileName = %s, const unsigned char* source= %p, size_t size = %lu, IOTHUB_CLIENT_FILE_UPLOAD_CALLBACK iotHubClientFileUploadCallback = %p, void* context = %p",
            iotHubClientHandle,
            destinationFileName,
            source,
            size,
            iotHubClientFileUploadCallback,
            context
        );
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        /*Codes_SRS_IOTHUBCLIENT_02_051: [IoTHubClient_UploadToBlobAsync shall copy the souce, size, iotHubClientFileUploadCallback, context into a structure.]*/
        UPLOADTOBLOB_SAVED_DATA *savedData = (UPLOADTOBLOB_SAVED_DATA *)malloc(sizeof(UPLOADTOBLOB_SAVED_DATA));
        if (savedData == NULL)
        {
            /*Codes_SRS_IOTHUBCLIENT_02_053: [ If copying to the structure or spawning the thread fails, then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_ERROR. ]*/
            LogError("unable to malloc - oom");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            if (mallocAndStrcpy_s((char**)&savedData->destinationFileName, destinationFileName) != 0)
            {
                /*Codes_SRS_IOTHUBCLIENT_02_053: [ If copying to the structure or spawning the thread fails, then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                LogError("unable to mallocAndStrcpy_s");
                free(savedData);
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                savedData->size = size;
                int sourceCloned;
                if (size == 0)
                {
                    savedData->source = NULL;
                    sourceCloned = 1;
                }
                else
                {
                    savedData->source = (unsigned char*)malloc(size);
                    if (savedData->source == NULL)
                    {
                        /*Codes_SRS_IOTHUBCLIENT_02_053: [ If copying to the structure or spawning the thread fails, then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                        LogError("unable to malloc - oom");
                        free(savedData->destinationFileName);
                        free(savedData);
                        sourceCloned = 0;
                    }
                    else
                    {
                        sourceCloned = 1;
                    }
                }

                if (sourceCloned == 0)
                {
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    IOTHUB_CLIENT_INSTANCE* iotHubClientHandleData = (IOTHUB_CLIENT_INSTANCE*)iotHubClientHandle;

                    savedData->iotHubClientFileUploadCallback = iotHubClientFileUploadCallback;
                    savedData->context = context;
                    (void)memcpy(savedData->source, source, size);

                    if ((result = StartWorkerThreadIfNeeded(iotHubClientHandleData)) != IOTHUB_CLIENT_OK)
                    {
                        free(savedData->source);
                        free(savedData->destinationFileName);
                        free(savedData);
                        result = IOTHUB_CLIENT_ERROR;
                        LogError("Could not start worker thread");
                    }
                    else
                    {
                        if (Lock(iotHubClientHandleData->LockHandle) != LOCK_OK) /*locking because the next statement is changing blobThreadsToBeJoined*/
                        {
                            LogError("unable to lock");
                            free(savedData->source);
                            free(savedData->destinationFileName);
                            free(savedData);
                            result = IOTHUB_CLIENT_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_IOTHUBCLIENT_02_058: [ IoTHubClient_UploadToBlobAsync shall add the structure to the list of structures that need to be cleaned once file upload finishes. ]*/
                            LIST_ITEM_HANDLE item = singlylinkedlist_add(iotHubClientHandleData->savedDataToBeCleaned, savedData);
                            if (item == NULL)
                            {
                                LogError("unable to singlylinkedlist_add");
                                free(savedData->source);
                                free(savedData->destinationFileName);
                                free(savedData);
                                result = IOTHUB_CLIENT_ERROR;
                            }
                            else
                            {
                                savedData->iotHubClientHandle = iotHubClientHandle;
                                savedData->canBeGarbageCollected = 0;
                                if ((savedData->lockGarbage = Lock_Init()) == NULL)
                                {
                                    (void)singlylinkedlist_remove(iotHubClientHandleData->savedDataToBeCleaned, item);
                                    free(savedData->source);
                                    free(savedData->destinationFileName);
                                    free(savedData);
                                    result = IOTHUB_CLIENT_ERROR;
                                    LogError("unable to Lock_Init");
                                }
                                else
                                {
                                    /*Codes_SRS_IOTHUBCLIENT_02_052: [ IoTHubClient_UploadToBlobAsync shall spawn a thread passing the structure build in SRS IOTHUBCLIENT 02 051 as thread data.]*/
                                    if (ThreadAPI_Create(&savedData->uploadingThreadHandle, uploadingThread, savedData) != THREADAPI_OK)
                                    {
                                        /*Codes_SRS_IOTHUBCLIENT_02_053: [ If copying to the structure or spawning the thread fails, then IoTHubClient_UploadToBlobAsync shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                                        LogError("unablet to ThreadAPI_Create");
                                        (void)Lock_Deinit(savedData->lockGarbage);
                                        (void)singlylinkedlist_remove(iotHubClientHandleData->savedDataToBeCleaned, item);
                                        free(savedData->source);
                                        free(savedData->destinationFileName);
                                        free(savedData);
                                        result = IOTHUB_CLIENT_ERROR;
                                    }
                                    else
                                    {
                                        result = IOTHUB_CLIENT_OK;
                                    }
                                }
                            }

                            (void)Unlock(iotHubClientHandleData->LockHandle);
                        }
                    }
                }
            }
        }
    }
    return result;
}
#endif /*USE_UPLOADTOBLOB*/

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>
#include "az_iot/c-utility/inc/azure_c_shared_utility/optimize_size.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/gballoc.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/string_tokenizer.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/doublylinkedlist.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/xlogging.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/tickcounter.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/constbuffer.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/platform.h"

#include "../inc/iothub_client_authorization.h"
#include "../inc/iothub_client_ll.h"
#include "../inc/iothub_transport_ll.h"
#include "../inc/iothub_client_private.h"
#include "../inc/iothub_client_options.h"
#include "../inc/iothub_client_version.h"
#include <stdint.h>

#ifdef USE_DPS_MODULE
#include "iothub_client_dps_ll.h"
#endif

#ifdef USE_UPLOADTOBLOB
#include "iothub_client_ll_uploadtoblob.h"
#endif

#define LOG_ERROR_RESULT LogError("result = %s", ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, result));
#define INDEFINITE_TIME ((time_t)(-1))

DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);

#define CALLBACK_TYPE_VALUES \
    CALLBACK_TYPE_NONE,      \
    CALLBACK_TYPE_SYNC,    \
    CALLBACK_TYPE_ASYNC

DEFINE_ENUM(CALLBACK_TYPE, CALLBACK_TYPE_VALUES)
DEFINE_ENUM_STRINGS(CALLBACK_TYPE, CALLBACK_TYPE_VALUES)

typedef struct IOTHUB_METHOD_CALLBACK_DATA_TAG
{
    CALLBACK_TYPE type;
    IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC callbackSync;
    IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK callbackAsync;
    void* userContextCallback;
}IOTHUB_METHOD_CALLBACK_DATA;

typedef struct IOTHUB_MESSAGE_CALLBACK_DATA_TAG
{
    CALLBACK_TYPE type;
    IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC callbackSync;
    IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX callbackAsync;
    void* userContextCallback;
}IOTHUB_MESSAGE_CALLBACK_DATA;

typedef struct IOTHUB_CLIENT_LL_HANDLE_DATA_TAG
{
    DLIST_ENTRY waitingToSend;
    DLIST_ENTRY iot_msg_queue;
    DLIST_ENTRY iot_ack_queue;
    TRANSPORT_LL_HANDLE transportHandle;
    bool isSharedTransport;
    IOTHUB_DEVICE_HANDLE deviceHandle;
    TRANSPORT_PROVIDER_FIELDS;
    IOTHUB_MESSAGE_CALLBACK_DATA messageCallback;
    IOTHUB_METHOD_CALLBACK_DATA methodCallback;
    IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK conStatusCallback;
    void* conStatusUserContextCallback;
    time_t lastMessageReceiveTime;
    TICK_COUNTER_HANDLE tickCounter; /*shared tickcounter used to track message timeouts in waitingToSend list*/
    tickcounter_ms_t currentMessageTimeout;
    uint64_t current_device_twin_timeout;
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback;
    void* deviceTwinContextCallback;
    IOTHUB_CLIENT_RETRY_POLICY retryPolicy;
    size_t retryTimeoutLimitInSeconds;
#ifdef USE_UPLOADTOBLOB
    IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE uploadToBlobHandle;
#endif
    uint32_t data_msg_id;
    bool complete_twin_update_encountered;
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;
    STRING_HANDLE product_info;
}IOTHUB_CLIENT_LL_HANDLE_DATA;

static const char HOSTNAME_TOKEN[] = "HostName";
static const char DEVICEID_TOKEN[] = "DeviceId";
static const char X509_TOKEN[] = "x509";
static const char X509_TOKEN_ONLY_ACCEPTABLE_VALUE[] = "true";
static const char DEVICEKEY_TOKEN[] = "SharedAccessKey";
static const char DEVICESAS_TOKEN[] = "SharedAccessSignature";
static const char PROTOCOL_GATEWAY_HOST[] = "GatewayHostName";

static void setTransportProtocol(IOTHUB_CLIENT_LL_HANDLE_DATA* handleData, TRANSPORT_PROVIDER* protocol)
{
    handleData->IoTHubTransport_SendMessageDisposition = protocol->IoTHubTransport_SendMessageDisposition;
    handleData->IoTHubTransport_GetHostname = protocol->IoTHubTransport_GetHostname;
    handleData->IoTHubTransport_SetOption = protocol->IoTHubTransport_SetOption;
    handleData->IoTHubTransport_Create = protocol->IoTHubTransport_Create;
    handleData->IoTHubTransport_Destroy = protocol->IoTHubTransport_Destroy;
    handleData->IoTHubTransport_Register = protocol->IoTHubTransport_Register;
    handleData->IoTHubTransport_Unregister = protocol->IoTHubTransport_Unregister;
    handleData->IoTHubTransport_Subscribe = protocol->IoTHubTransport_Subscribe;
    handleData->IoTHubTransport_Unsubscribe = protocol->IoTHubTransport_Unsubscribe;
    handleData->IoTHubTransport_DoWork = protocol->IoTHubTransport_DoWork;
    handleData->IoTHubTransport_SetRetryPolicy = protocol->IoTHubTransport_SetRetryPolicy;
    handleData->IoTHubTransport_GetSendStatus = protocol->IoTHubTransport_GetSendStatus;
    handleData->IoTHubTransport_ProcessItem = protocol->IoTHubTransport_ProcessItem;
    handleData->IoTHubTransport_Subscribe_DeviceTwin = protocol->IoTHubTransport_Subscribe_DeviceTwin;
    handleData->IoTHubTransport_Unsubscribe_DeviceTwin = protocol->IoTHubTransport_Unsubscribe_DeviceTwin;
    handleData->IoTHubTransport_Subscribe_DeviceMethod = protocol->IoTHubTransport_Subscribe_DeviceMethod;
    handleData->IoTHubTransport_Unsubscribe_DeviceMethod = protocol->IoTHubTransport_Unsubscribe_DeviceMethod;
    handleData->IoTHubTransport_DeviceMethod_Response = protocol->IoTHubTransport_DeviceMethod_Response;
}

static void device_twin_data_destroy(IOTHUB_DEVICE_TWIN* client_item)
{
    CONSTBUFFER_Destroy(client_item->report_data_handle);
    free(client_item);
}

static int create_blob_upload_module(IOTHUB_CLIENT_LL_HANDLE_DATA* handle_data, const IOTHUB_CLIENT_CONFIG* config)
{
    int result;
    (void)handle_data;
    (void)config;
#ifdef USE_UPLOADTOBLOB
    handle_data->uploadToBlobHandle = IoTHubClient_LL_UploadToBlob_Create(config);
    if (handle_data->uploadToBlobHandle == NULL)
    {
        LogError("unable to IoTHubClient_LL_UploadToBlob_Create");
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }
#else
    result = 0;
#endif
    return result;
}

static void destroy_blob_upload_module(IOTHUB_CLIENT_LL_HANDLE_DATA* handle_data)
{
    (void)handle_data;
#ifdef USE_UPLOADTOBLOB
    /*Codes_SRS_IOTHUBCLIENT_LL_02_046: [ If creating the TICK_COUNTER_HANDLE fails then IoTHubClient_LL_Create shall fail and return NULL. ]*/
    IoTHubClient_LL_UploadToBlob_Destroy(handle_data->uploadToBlobHandle);
#endif
}

/*Codes_SRS_IOTHUBCLIENT_LL_10_032: ["product_info" - takes a char string as an argument to specify the product information(e.g. `"ProductName/ProductVersion"`). ]*/
/*Codes_SRS_IOTHUBCLIENT_LL_10_034: ["product_info" - shall store the given string concatenated with the sdk information and the platform information in the form(ProductInfo DeviceSDKName / DeviceSDKVersion(OSName OSVersion; Architecture). ]*/
static STRING_HANDLE make_product_info(const char* product)
{
    STRING_HANDLE result;
    STRING_HANDLE pfi = platform_get_platform_info();
    if (pfi == NULL)
    {
        result = NULL;
    }
    else
    {
        if (product == NULL)
        {
            result = STRING_construct_sprintf("%s %s", CLIENT_DEVICE_TYPE_PREFIX CLIENT_DEVICE_BACKSLASH IOTHUB_SDK_VERSION, STRING_c_str(pfi));
        }
        else
        {
            result = STRING_construct_sprintf("%s %s %s", product, CLIENT_DEVICE_TYPE_PREFIX CLIENT_DEVICE_BACKSLASH IOTHUB_SDK_VERSION, STRING_c_str(pfi));
        }
        STRING_delete(pfi);
    }
    return result;
}

static IOTHUB_CLIENT_LL_HANDLE_DATA* initialize_iothub_client(const IOTHUB_CLIENT_CONFIG* client_config, const IOTHUB_CLIENT_DEVICE_CONFIG* device_config, bool use_dev_auth)
{
    IOTHUB_CLIENT_LL_HANDLE_DATA* result;
    STRING_HANDLE product_info = make_product_info(NULL);
    if (product_info == NULL)
    {
        LogError("failed to initialize product info");
        result = NULL;
    }
    else
    {
        result = (IOTHUB_CLIENT_LL_HANDLE_DATA*)malloc(sizeof(IOTHUB_CLIENT_LL_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("failure allocating IOTHUB_CLIENT_LL_HANDLE_DATA");
            STRING_delete(product_info);
        }
        else
        {
            IOTHUB_CLIENT_CONFIG actual_config;
            const IOTHUB_CLIENT_CONFIG* config = NULL;
            char* IoTHubName = NULL;
            char* IoTHubSuffix = NULL;

            memset(result, 0, sizeof(IOTHUB_CLIENT_LL_HANDLE_DATA));
            if (use_dev_auth)
            {
                if ((result->authorization_module = IoTHubClient_Auth_CreateFromDeviceAuth(client_config->deviceId)) == NULL)
                {
                    LogError("Failed create authorization module");
                    free(result);
                    STRING_delete(product_info);
                    result = NULL;
                }
            }
            else
            {
                const char* device_key;
                const char* device_id;
                const char* sas_token;
                if (device_config == NULL)
                {
                    device_key = client_config->deviceKey;
                    device_id = client_config->deviceId;
                    sas_token = client_config->deviceSasToken;
                }
                else
                {
                    device_key = device_config->deviceKey;
                    device_id = device_config->deviceId;
                    sas_token = device_config->deviceSasToken;
                }

                /* Codes_SRS_IOTHUBCLIENT_LL_07_029: [ IoTHubClient_LL_Create shall create the Auth module with the device_key, device_id, and/or deviceSasToken values ] */
                if ((result->authorization_module = IoTHubClient_Auth_Create(device_key, device_id, sas_token)) == NULL)
                {
                    LogError("Failed create authorization module");
                    free(result);
                    STRING_delete(product_info);
                    result = NULL;
                }
            }

            if (result != NULL)
            {
                if (client_config != NULL)
                {
                    IOTHUBTRANSPORT_CONFIG lowerLayerConfig;
                    memset(&lowerLayerConfig, 0, sizeof(IOTHUBTRANSPORT_CONFIG));
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_006: [IoTHubClient_LL_Create shall populate a structure of type IOTHUBTRANSPORT_CONFIG with the information from config parameter and the previous DLIST and shall pass that to the underlying layer _Create function.]*/
                    lowerLayerConfig.upperConfig = client_config;
                    lowerLayerConfig.waitingToSend = &(result->waitingToSend);
                    lowerLayerConfig.auth_module_handle = result->authorization_module;

                    setTransportProtocol(result, (TRANSPORT_PROVIDER*)client_config->protocol());
                    if ((result->transportHandle = result->IoTHubTransport_Create(&lowerLayerConfig)) == NULL)
                    {
                        /*Codes_SRS_IOTHUBCLIENT_LL_02_007: [If the underlaying layer _Create function fails them IoTHubClient_LL_Create shall fail and return NULL.] */
                        LogError("underlying transport failed");
                        destroy_blob_upload_module(result);
                        tickcounter_destroy(result->tickCounter);
                        IoTHubClient_Auth_Destroy(result->authorization_module);
                        STRING_delete(product_info);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBCLIENT_LL_02_008: [Otherwise, IoTHubClient_LL_Create shall succeed and return a non-NULL handle.] */
                        result->isSharedTransport = false;
                        config = client_config;
                    }
                }
                else
                {
                    STRING_HANDLE transport_hostname = NULL;

                    result->transportHandle = device_config->transportHandle;
                    setTransportProtocol(result, (TRANSPORT_PROVIDER*)device_config->protocol());

                    transport_hostname = result->IoTHubTransport_GetHostname(result->transportHandle);
                    if (transport_hostname == NULL)
                    {
                        /*Codes_SRS_IOTHUBCLIENT_LL_02_097: [ If creating the data structures fails or instantiating the IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE fails then IoTHubClient_LL_CreateWithTransport shall fail and return NULL. ]*/
                        LogError("unable to determine the transport IoTHub name");
                        IoTHubClient_Auth_Destroy(result->authorization_module);
                        STRING_delete(product_info);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        const char* hostname = STRING_c_str(transport_hostname);
                        /*Codes_SRS_IOTHUBCLIENT_LL_02_096: [ IoTHubClient_LL_CreateWithTransport shall create the data structures needed to instantiate a IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE. ]*/
                        /*the first '.' says where the iothubname finishes*/
                        const char* whereIsDot = strchr(hostname, '.');
                        if (whereIsDot == NULL)
                        {
                            /*Codes_SRS_IOTHUBCLIENT_LL_02_097: [ If creating the data structures fails or instantiating the IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE fails then IoTHubClient_LL_CreateWithTransport shall fail and return NULL. ]*/
                            LogError("unable to determine the IoTHub name");
                            IoTHubClient_Auth_Destroy(result->authorization_module);
                            STRING_delete(product_info);
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            size_t suffix_len = strlen(whereIsDot);
                            /*Codes_SRS_IOTHUBCLIENT_LL_02_096: [ IoTHubClient_LL_CreateWithTransport shall create the data structures needed to instantiate a IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE. ]*/
                            IoTHubName = (char*)malloc(whereIsDot - hostname + 1);
                            if (IoTHubName == NULL)
                            {
                                /*Codes_SRS_IOTHUBCLIENT_LL_02_097: [ If creating the data structures fails or instantiating the IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE fails then IoTHubClient_LL_CreateWithTransport shall fail and return NULL. ]*/
                                LogError("unable to malloc");
                                IoTHubClient_Auth_Destroy(result->authorization_module);
                                STRING_delete(product_info);
                                free(result);
                                result = NULL;
                            }
                            else if ((IoTHubSuffix = (char*)malloc(suffix_len + 1)) == NULL)
                            {
                                /*Codes_SRS_IOTHUBCLIENT_LL_02_097: [ If creating the data structures fails or instantiating the IOTHUB_CLIENT_LL_UPLOADTOBLOB_HANDLE fails then IoTHubClient_LL_CreateWithTransport shall fail and return NULL. ]*/
                                LogError("unable to malloc");
                                IoTHubClient_Auth_Destroy(result->authorization_module);
                                STRING_delete(product_info);
                                free(result);
                                result = NULL;
                            }
                            else
                            {
                                memset(IoTHubName, 0, whereIsDot - hostname + 1);
                                (void)strncpy(IoTHubName, hostname, whereIsDot - hostname);
                                (void)strcpy(IoTHubSuffix, whereIsDot+1);

                                actual_config.deviceId = device_config->deviceId;
                                actual_config.deviceKey = device_config->deviceKey;
                                actual_config.deviceSasToken = device_config->deviceSasToken;
                                actual_config.iotHubName = IoTHubName;
                                actual_config.iotHubSuffix = IoTHubSuffix;
                                actual_config.protocol = NULL; /*irrelevant to IoTHubClient_LL_UploadToBlob*/
                                actual_config.protocolGatewayHostName = NULL; /*irrelevant to IoTHubClient_LL_UploadToBlob*/

                                config = &actual_config;

                                /*Codes_SRS_IOTHUBCLIENT_LL_02_008: [Otherwise, IoTHubClient_LL_Create shall succeed and return a non-NULL handle.] */
                                result->isSharedTransport = true;
                            }
                        }
                    }
                    STRING_delete(transport_hostname);
                }
            }
            if (result != NULL)
            {
                if (create_blob_upload_module(result, config) != 0)
                {
                    LogError("unable to create blob upload");
                    // Codes_SRS_IOTHUBCLIENT_LL_09_010: [ If any failure occurs `IoTHubClient_LL_Create` shall destroy the `transportHandle` only if it has created it ]
                    if (!result->isSharedTransport)
                    {
                        result->IoTHubTransport_Destroy(result->transportHandle);
                    }
                    IoTHubClient_Auth_Destroy(result->authorization_module);
                    STRING_delete(product_info);
                    free(result);
                    result = NULL;
                }
                else
                {
                    if ((result->tickCounter = tickcounter_create()) == NULL)
                    {
                        LogError("unable to get a tickcounter");
                        // Codes_SRS_IOTHUBCLIENT_LL_09_010: [ If any failure occurs `IoTHubClient_LL_Create` shall destroy the `transportHandle` only if it has created it ]
                        if (!result->isSharedTransport)
                        {
                            result->IoTHubTransport_Destroy(result->transportHandle);
                        }
                        destroy_blob_upload_module(result);
                        IoTHubClient_Auth_Destroy(result->authorization_module);
                        STRING_delete(product_info);
                        free(result);
                        result = NULL;
                    }
                    else
                    {
                        /*Codes_SRS_IOTHUBCLIENT_LL_02_004: [Otherwise IoTHubClient_LL_Create shall initialize a new DLIST (further called "waitingToSend") containing records with fields of the following types: IOTHUB_MESSAGE_HANDLE, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, void*.]*/
                        DList_InitializeListHead(&(result->waitingToSend));
                        DList_InitializeListHead(&(result->iot_msg_queue));
                        DList_InitializeListHead(&(result->iot_ack_queue));
                        result->messageCallback.type = CALLBACK_TYPE_NONE;
                        result->lastMessageReceiveTime = INDEFINITE_TIME;
                        result->data_msg_id = 1;
                        result->product_info = product_info;

                        IOTHUB_DEVICE_CONFIG deviceConfig;
                        deviceConfig.deviceId = config->deviceId;
                        deviceConfig.deviceKey = config->deviceKey;
                        deviceConfig.deviceSasToken = config->deviceSasToken;
                        deviceConfig.authorization_module = result->authorization_module;

                        /*Codes_SRS_IOTHUBCLIENT_LL_17_008: [IoTHubClient_LL_Create shall call the transport _Register function with a populated structure of type IOTHUB_DEVICE_CONFIG and waitingToSend list.] */
                        if ((result->deviceHandle = result->IoTHubTransport_Register(result->transportHandle, &deviceConfig, result, &(result->waitingToSend))) == NULL)
                        {
                            LogError("Registering device in transport failed");
                            IoTHubClient_Auth_Destroy(result->authorization_module);
                            // Codes_SRS_IOTHUBCLIENT_LL_09_010: [ If any failure occurs `IoTHubClient_LL_Create` shall destroy the `transportHandle` only if it has created it ]
                            if (!result->isSharedTransport)
                            {
                                result->IoTHubTransport_Destroy(result->transportHandle);
                            }
                            destroy_blob_upload_module(result);
                            tickcounter_destroy(result->tickCounter);
                            STRING_delete(product_info);
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            /*Codes_SRS_IOTHUBCLIENT_LL_02_042: [ By default, messages shall not timeout. ]*/
                            result->currentMessageTimeout = 0;
                            result->current_device_twin_timeout = 0;
                            /*Codes_SRS_IOTHUBCLIENT_LL_25_124: [ `IoTHubClient_LL_Create` shall set the default retry policy as Exponential backoff with jitter and if succeed and return a `non-NULL` handle. ]*/
                            if (IoTHubClient_LL_SetRetryPolicy(result, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER, 0) != IOTHUB_CLIENT_OK)
                            {
                                LogError("Setting default retry policy in transport failed");
                                result->IoTHubTransport_Unregister(result->deviceHandle);
                                IoTHubClient_Auth_Destroy(result->authorization_module);
                                // Codes_SRS_IOTHUBCLIENT_LL_09_010: [ If any failure occurs `IoTHubClient_LL_Create` shall destroy the `transportHandle` only if it has created it ]
                                if (!result->isSharedTransport)
                                {
                                    result->IoTHubTransport_Destroy(result->transportHandle);
                                }
                                destroy_blob_upload_module(result);
                                tickcounter_destroy(result->tickCounter);
                                STRING_delete(product_info);
                                free(result);
                                result = NULL;
                            }
                        }
                    }
                }
            }
            if (IoTHubName)
            {
                free(IoTHubName);
            }
            if (IoTHubSuffix)
            {
                free(IoTHubSuffix);
            }
        }
    }
    return result;
}

static uint32_t get_next_item_id(IOTHUB_CLIENT_LL_HANDLE_DATA* handleData)
{    
    if (handleData->data_msg_id+1 >= UINT32_MAX)
    {
        handleData->data_msg_id = 1;
    }
    else
    {
        handleData->data_msg_id++;
    }
    return handleData->data_msg_id;
}

static IOTHUB_DEVICE_TWIN* dev_twin_data_create(IOTHUB_CLIENT_LL_HANDLE_DATA* handleData, uint32_t id, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    IOTHUB_DEVICE_TWIN* result = (IOTHUB_DEVICE_TWIN*)malloc(sizeof(IOTHUB_DEVICE_TWIN) );
    if (result != NULL)
    {
        result->report_data_handle = CONSTBUFFER_Create(reportedState, size);
        if (result->report_data_handle == NULL)
        {
            LogError("Failure allocating reported state data");
            free(result);
            result = NULL;
        }
        else if (tickcounter_get_current_ms(handleData->tickCounter, &result->ms_timesOutAfter) != 0)
        {
            LogError("Failure getting tickcount info");
            CONSTBUFFER_Destroy(result->report_data_handle);
            free(result);
            result = NULL;
        }
        else
        {
            result->item_id = id;
            result->ms_timesOutAfter = 0;
            result->context = userContextCallback;
            result->reported_state_callback = reportedStateCallback;
            result->client_handle = handleData;
            result->device_handle = handleData->deviceHandle;
        }
    }
    else
    {
        LogError("Failure allocating device twin information");
    }
    return result;
}

IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateFromDeviceAuth(const char* iothub_uri, const char* device_id, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_LL_HANDLE result;
    if (iothub_uri == NULL || protocol == NULL || device_id == NULL)
    {
        LogError("Input parameter is NULL: iothub_uri: %p  protocol: %p device_id: %p", iothub_uri, protocol, device_id);
        result = NULL;
    }
    else
    {
#ifdef USE_DPS_MODULE
        IOTHUB_CLIENT_CONFIG* config = (IOTHUB_CLIENT_CONFIG*)malloc(sizeof(IOTHUB_CLIENT_CONFIG));
        if (config == NULL)
        {
            /* Codes_SRS_IOTHUBCLIENT_LL_12_012: [If the allocation failed IoTHubClient_LL_CreateFromConnectionString  returns NULL]  */
            LogError("Malloc failed");
            result = NULL;
        }
        else
        {
            const char* iterator;
            const char* initial;
            char* iothub_name = NULL;
            char* iothub_suffix = NULL;

            memset(config, 0, sizeof(IOTHUB_CLIENT_CONFIG) );
            config->protocol = protocol;
            config->deviceId = device_id;
            //config->useDeviceAuthKey = 1;
            
            // Find the iothub suffix
            initial = iterator = iothub_uri;
            while (iterator != NULL && *iterator != '\0')
            {
                if (*iterator == '.')
                {
                    size_t length = iterator-initial;
                    iothub_name = (char*)malloc(length+1);
                    if (iothub_name != NULL)
                    {
                        memset(iothub_name, 0, length + 1);
                        memcpy(iothub_name, initial, length);
                        config->iotHubName = iothub_name;

                        length = strlen(initial)-length-1;
                        iothub_suffix = (char*)malloc(length + 1);
                        if (iothub_suffix != NULL)
                        {
                            memset(iothub_suffix, 0, length + 1);
                            memcpy(iothub_suffix, iterator+1, length);
                            config->iotHubSuffix = iothub_suffix;
                            break;
                        }
                        else
                        {
                            LogError("Failed to allocate iothub suffix");
                            free(iothub_name);
                            result = NULL;
                        }
                    }
                    else
                    {
                        LogError("Failed to allocate iothub name");
                        result = NULL;
                    }
                }
                iterator++;
            }

            if (config->iotHubName == NULL || config->iotHubSuffix == NULL)
            {
                LogError("initialize iothub client");
                result = NULL;
            }
            else
            {
                IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = initialize_iothub_client(config, NULL, true);
                if (handleData == NULL)
                {
                    LogError("initialize iothub client");
                    result = NULL;
                }
                else
                {
                    result = handleData;
                }
            }

            free(iothub_name);
            free(iothub_suffix);
            free(config);
        }
#else
        LogError("DPS module is not included");
        result = NULL;
#endif
    }
    return result;
}

IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateFromConnectionString(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_LL_HANDLE result;

    /*Codes_SRS_IOTHUBCLIENT_LL_05_001: [IoTHubClient_LL_CreateFromConnectionString shall obtain the version string by a call to IoTHubClient_GetVersionString.]*/
    /*Codes_SRS_IOTHUBCLIENT_LL_05_002: [IoTHubClient_LL_CreateFromConnectionString shall print the version string to standard output.]*/
    LogInfo("IoT Hub SDK for C, version %s", IoTHubClient_GetVersionString());

    /* Codes_SRS_IOTHUBCLIENT_LL_12_003: [IoTHubClient_LL_CreateFromConnectionString shall verify the input parameter and if it is NULL then return NULL] */
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
        /* Codes_SRS_IOTHUBCLIENT_LL_12_004: [IoTHubClient_LL_CreateFromConnectionString shall allocate IOTHUB_CLIENT_CONFIG structure] */
        IOTHUB_CLIENT_CONFIG* config = (IOTHUB_CLIENT_CONFIG*) malloc(sizeof(IOTHUB_CLIENT_CONFIG));
        if (config == NULL)
        {
            /* Codes_SRS_IOTHUBCLIENT_LL_12_012: [If the allocation failed IoTHubClient_LL_CreateFromConnectionString  returns NULL]  */
            LogError("Malloc failed");
            result = NULL;
        }
        else
        {
            STRING_TOKENIZER_HANDLE tokenizer1 = NULL;
            STRING_HANDLE connString = NULL;
            STRING_HANDLE tokenString = NULL;
            STRING_HANDLE valueString = NULL;
            STRING_HANDLE hostNameString = NULL;
            STRING_HANDLE hostSuffixString = NULL;
            STRING_HANDLE deviceIdString = NULL;
            STRING_HANDLE deviceKeyString = NULL;
            STRING_HANDLE deviceSasTokenString = NULL;
            STRING_HANDLE protocolGateway = NULL;

            config->protocol = protocol;

            config->iotHubName = NULL;
            config->iotHubSuffix = NULL;
            config->deviceId = NULL;
            config->deviceKey = NULL;
            config->deviceSasToken = NULL;

            /* Codes_SRS_IOTHUBCLIENT_LL_04_002: [If it does not, it shall pass the protocolGatewayHostName NULL.] */
            config->protocolGatewayHostName = NULL;

            if ((connString = STRING_construct(connectionString)) == NULL)
            {
                LogError("Error constructing connectiong String");
                result = NULL;
            }
            else if ((tokenizer1 = STRING_TOKENIZER_create(connString)) == NULL)
            {
                LogError("Error creating Tokenizer");
                result = NULL;
            }
            else if ((tokenString = STRING_new()) == NULL)
            {
                LogError("Error creating Token String");
                result = NULL;
            }
            else if ((valueString = STRING_new()) == NULL)
            {
                LogError("Error creating Value String");
                result = NULL;
            }
            else if ((hostNameString = STRING_new()) == NULL)
            {
                LogError("Error creating HostName String");
                result = NULL;
            }
            else if ((hostSuffixString = STRING_new()) == NULL)
            {
                LogError("Error creating HostSuffix String");
                result = NULL;
            }
            /* Codes_SRS_IOTHUBCLIENT_LL_12_005: [IoTHubClient_LL_CreateFromConnectionString shall try to parse the connectionString input parameter for the following structure: "Key1=value1;key2=value2;key3=value3..."] */
            /* Codes_SRS_IOTHUBCLIENT_LL_12_006: [IoTHubClient_LL_CreateFromConnectionString shall verify the existence of the following Key/Value pairs in the connection string: HostName, DeviceId, SharedAccessKey, SharedAccessSignature or x509]  */
            else
            {
                int isx509found = 0;
                while ((STRING_TOKENIZER_get_next_token(tokenizer1, tokenString, "=") == 0))
                {
                    if (STRING_TOKENIZER_get_next_token(tokenizer1, valueString, ";") != 0)
                    {
                        LogError("Tokenizer error");
                        break;
                    }
                    else
                    {
                        if (tokenString != NULL)
                        {
                            /* Codes_SRS_IOTHUBCLIENT_LL_12_010: [IoTHubClient_LL_CreateFromConnectionString shall fill up the IOTHUB_CLIENT_CONFIG structure using the following mapping: iotHubName = Name, iotHubSuffix = Suffix, deviceId = DeviceId, deviceKey = SharedAccessKey or deviceSasToken = SharedAccessSignature] */
                            const char* s_token = STRING_c_str(tokenString);
                            if (strcmp(s_token, HOSTNAME_TOKEN) == 0)
                            {
                                /* Codes_SRS_IOTHUBCLIENT_LL_12_009: [IoTHubClient_LL_CreateFromConnectionString shall split the value of HostName to Name and Suffix using the first "." as a separator] */
                                STRING_TOKENIZER_HANDLE tokenizer2 = NULL;
                                if ((tokenizer2 = STRING_TOKENIZER_create(valueString)) == NULL)
                                {
                                    LogError("Error creating Tokenizer");
                                    break;
                                }
                                else
                                {
                                    /* Codes_SRS_IOTHUBCLIENT_LL_12_015: [If the string split failed, IoTHubClient_LL_CreateFromConnectionString returns NULL ] */
                                    if (STRING_TOKENIZER_get_next_token(tokenizer2, hostNameString, ".") != 0)
                                    {
                                        LogError("Tokenizer error");
                                        STRING_TOKENIZER_destroy(tokenizer2);
                                        break;
                                    }
                                    else
                                    {
                                        config->iotHubName = STRING_c_str(hostNameString);
                                        if (STRING_TOKENIZER_get_next_token(tokenizer2, hostSuffixString, ";") != 0)
                                        {
                                            LogError("Tokenizer error");
                                            STRING_TOKENIZER_destroy(tokenizer2);
                                            break;
                                        }
                                        else
                                        {
                                            config->iotHubSuffix = STRING_c_str(hostSuffixString);
                                        }
                                    }
                                    STRING_TOKENIZER_destroy(tokenizer2);
                                }
                            }
                            else if (strcmp(s_token, DEVICEID_TOKEN) == 0)
                            {
                                deviceIdString = STRING_clone(valueString);
                                if (deviceIdString != NULL)
                                {
                                    config->deviceId = STRING_c_str(deviceIdString);
                                }
                                else
                                {
                                    LogError("Failure cloning device id string");
                                    break;
                                }
                            }
                            else if (strcmp(s_token, DEVICEKEY_TOKEN) == 0)
                            {
                                deviceKeyString = STRING_clone(valueString);
                                if (deviceKeyString != NULL)
                                {
                                    config->deviceKey = STRING_c_str(deviceKeyString);
                                }
                                else
                                {
                                    LogError("Failure cloning device key string");
                                    break;
                                }
                            }
                            else if (strcmp(s_token, DEVICESAS_TOKEN) == 0)
                            {
                                deviceSasTokenString = STRING_clone(valueString);
                                if (deviceSasTokenString != NULL)
                                {
                                    config->deviceSasToken = STRING_c_str(deviceSasTokenString);
                                }
                                else
                                {
                                    LogError("Failure cloning device sasToken string");
                                    break;
                                }
                            }
                            else if (strcmp(s_token, X509_TOKEN) == 0)
                            {
                                if (strcmp(STRING_c_str(valueString), X509_TOKEN_ONLY_ACCEPTABLE_VALUE) != 0)
                                {
                                    LogError("x509 option has wrong value, the only acceptable one is \"true\"");
                                    break;
                                }
                                else
                                {
                                    isx509found = 1;
                                }
                            }
                            /* Codes_SRS_IOTHUBCLIENT_LL_04_001: [IoTHubClient_LL_CreateFromConnectionString shall verify the existence of key/value pair GatewayHostName. If it does exist it shall pass the value to IoTHubClient_LL_Create API.] */
                            else if (strcmp(s_token, PROTOCOL_GATEWAY_HOST) == 0)
                            {
                                protocolGateway = STRING_clone(valueString);
                                if (protocolGateway != NULL)
                                {
                                    config->protocolGatewayHostName = STRING_c_str(protocolGateway);
                                }
                                else
                                {
                                    LogError("Failure cloning protocol Gateway Name");
                                    break;
                                }
                            }
                        }
                    }
                }
                /* parsing is done - check the result */
                if (config->iotHubName == NULL)
                {
                    LogError("iotHubName is not found");
                    result = NULL;
                }
                else if (config->iotHubSuffix == NULL)
                {
                    LogError("iotHubSuffix is not found");
                    result = NULL;
                }
                else if (config->deviceId == NULL)
                {
                    LogError("deviceId is not found");
                    result = NULL;
                }
                else if (!(
                    ((!isx509found) && (config->deviceSasToken == NULL) ^ (config->deviceKey == NULL)) ||
                    ((isx509found) && (config->deviceSasToken == NULL) && (config->deviceKey == NULL))
                    ))
                {
                    LogError("invalid combination of x509, deviceSasToken and deviceKey");
                    result = NULL;
                }
                else
                {
                    /* Codes_SRS_IOTHUBCLIENT_LL_12_011: [IoTHubClient_LL_CreateFromConnectionString shall call into the IoTHubClient_LL_Create API with the current structure and returns with the return value of it] */
                    result = initialize_iothub_client(config, NULL, false);
                    if (result == NULL)
                    {
                        LogError("IoTHubClient_LL_Create failed");
                    }
                    else
                    {
                        /*return as is*/
                    }
                }
            }
            if (deviceSasTokenString != NULL)
                STRING_delete(deviceSasTokenString);
            if (deviceKeyString != NULL)
                STRING_delete(deviceKeyString);
            if (deviceIdString != NULL)
                STRING_delete(deviceIdString);
            if (hostSuffixString != NULL)
                STRING_delete(hostSuffixString);
            if (hostNameString != NULL)
                STRING_delete(hostNameString);
            if (valueString != NULL)
                STRING_delete(valueString);
            if (tokenString != NULL)
                STRING_delete(tokenString);
            if (connString != NULL)
                STRING_delete(connString);
            if (protocolGateway != NULL)
                STRING_delete(protocolGateway);

            if (tokenizer1 != NULL)
                STRING_TOKENIZER_destroy(tokenizer1);

            free(config);
        }
    }
    return result;
}

IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_Create(const IOTHUB_CLIENT_CONFIG* config)
{
    IOTHUB_CLIENT_LL_HANDLE result;
    /*Codes_SRS_IOTHUBCLIENT_LL_02_001: [IoTHubClient_LL_Create shall return NULL if config parameter is NULL or protocol field is NULL.]*/
    if( 
        (config == NULL) ||
        (config->protocol == NULL)
        )
    {
        result = NULL;
        LogError("invalid configuration (NULL detected)");
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = initialize_iothub_client(config, NULL, false);
        if (handleData == NULL)
        {
            LogError("initialize iothub client");
            result = NULL;
        }
        else
        {
            result = handleData;
        }
    }

    return result;
}

IOTHUB_CLIENT_LL_HANDLE IoTHubClient_LL_CreateWithTransport(const IOTHUB_CLIENT_DEVICE_CONFIG * config)
{
    IOTHUB_CLIENT_LL_HANDLE result;
    /*Codes_SRS_IOTHUBCLIENT_LL_17_001: [IoTHubClient_LL_CreateWithTransport shall return NULL if config parameter is NULL, or protocol field is NULL or transportHandle is NULL.]*/
    if (
        (config == NULL) ||
        (config->protocol == NULL) ||
        (config->transportHandle == NULL) ||
        /*Codes_SRS_IOTHUBCLIENT_LL_02_098: [ IoTHubClient_LL_CreateWithTransport shall fail and return NULL if both config->deviceKey AND config->deviceSasToken are NULL. ]*/
        ((config->deviceKey == NULL) && (config->deviceSasToken == NULL))
        )
    {
        result = NULL;
        LogError("invalid configuration (NULL detected)");
    }
    else
    {
        result = initialize_iothub_client(NULL, config, false);
    }
    return result;
}

void IoTHubClient_LL_Destroy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
{
    /*Codes_SRS_IOTHUBCLIENT_LL_02_009: [IoTHubClient_LL_Destroy shall do nothing if parameter iotHubClientHandle is NULL.]*/
    if (iotHubClientHandle != NULL)
    {
        PDLIST_ENTRY unsend;
        /*Codes_SRS_IOTHUBCLIENT_LL_17_010: [IoTHubClient_LL_Destroy  shall call the underlaying layer's _Unregister function] */
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        handleData->IoTHubTransport_Unregister(handleData->deviceHandle);
        if (handleData->isSharedTransport == false)
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_02_010: [If iotHubClientHandle was not created by IoTHubClient_LL_CreateWithTransport, IoTHubClient_LL_Destroy  shall call the underlaying layer's _Destroy function.] */
            handleData->IoTHubTransport_Destroy(handleData->transportHandle);
        }
        /*if any, remove the items currently not send*/
        while ((unsend = DList_RemoveHeadList(&(handleData->waitingToSend))) != &(handleData->waitingToSend))
        {
            IOTHUB_MESSAGE_LIST* temp = containingRecord(unsend, IOTHUB_MESSAGE_LIST, entry);
            /*Codes_SRS_IOTHUBCLIENT_LL_02_033: [Otherwise, IoTHubClient_LL_Destroy shall complete all the event message callbacks that are in the waitingToSend list with the result IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY.] */
            if (temp->callback != NULL)
            {
                temp->callback(IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY, temp->context);
            }
            IoTHubMessage_Destroy(temp->messageHandle);
            free(temp);
        }

        /* Codes_SRS_IOTHUBCLIENT_LL_07_007: [ IoTHubClient_LL_Destroy shall iterate the device twin queues and destroy any remaining items. ] */
        while ((unsend = DList_RemoveHeadList(&(handleData->iot_msg_queue))) != &(handleData->iot_msg_queue))
        {
            IOTHUB_DEVICE_TWIN* temp = containingRecord(unsend, IOTHUB_DEVICE_TWIN, entry);
            device_twin_data_destroy(temp);
        }
        while ((unsend = DList_RemoveHeadList(&(handleData->iot_ack_queue))) != &(handleData->iot_ack_queue))
        {
            IOTHUB_DEVICE_TWIN* temp = containingRecord(unsend, IOTHUB_DEVICE_TWIN, entry);
            device_twin_data_destroy(temp);
        }

        /*Codes_SRS_IOTHUBCLIENT_LL_17_011: [IoTHubClient_LL_Destroy  shall free the resources allocated by IoTHubClient (if any).] */
        IoTHubClient_Auth_Destroy(handleData->authorization_module);
        tickcounter_destroy(handleData->tickCounter);
#ifdef USE_UPLOADTOBLOB
        IoTHubClient_LL_UploadToBlob_Destroy(handleData->uploadToBlobHandle);
#endif
        STRING_delete(handleData->product_info);
        free(handleData);
    }
}

/*Codes_SRS_IOTHUBCLIENT_LL_02_044: [ Messages already delivered to IoTHubClient_LL shall not have their timeouts modified by a new call to IoTHubClient_LL_SetOption. ]*/
/*returns 0 on success, any other value is error*/
static int attach_ms_timesOutAfter(IOTHUB_CLIENT_LL_HANDLE_DATA* handleData, IOTHUB_MESSAGE_LIST *newEntry)
{
    int result;
    /*Codes_SRS_IOTHUBCLIENT_LL_02_043: [ Calling IoTHubClient_LL_SetOption with value set to "0" shall disable the timeout mechanism for all new messages. ]*/
    if (handleData->currentMessageTimeout == 0)
    {
        newEntry->ms_timesOutAfter = 0; /*do not timeout*/
        result = 0;
    }
    else
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a uint64. ]*/
        if (tickcounter_get_current_ms(handleData->tickCounter, &newEntry->ms_timesOutAfter) != 0)
        {
            result = __FAILURE__;
            LogError("unable to get the current relative tickcount");
        }
        else
        {
            newEntry->ms_timesOutAfter += handleData->currentMessageTimeout;
            result = 0;
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendEventAsync(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_MESSAGE_HANDLE eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    /*Codes_SRS_IOTHUBCLIENT_LL_02_011: [IoTHubClient_LL_SendEventAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle or eventMessageHandle is NULL.]*/
    if (
        (iotHubClientHandle == NULL) ||
        (eventMessageHandle == NULL) ||
        /*Codes_SRS_IOTHUBCLIENT_LL_02_012: [IoTHubClient_LL_SendEventAsync shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter eventConfirmationCallback is NULL and userContextCallback is not NULL.] */
        ((eventConfirmationCallback == NULL) && (userContextCallback != NULL))
        )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_MESSAGE_LIST *newEntry = (IOTHUB_MESSAGE_LIST*)malloc(sizeof(IOTHUB_MESSAGE_LIST));
        if (newEntry == NULL)
        {
            result = IOTHUB_CLIENT_ERROR;
            LOG_ERROR_RESULT;
        }
        else
        {
            IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;

            if (attach_ms_timesOutAfter(handleData, newEntry) != 0)
            {
                result = IOTHUB_CLIENT_ERROR;
                LOG_ERROR_RESULT;
                free(newEntry);
            }
            else
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_02_013: [IoTHubClient_LL_SendEventAsync shall add the DLIST waitingToSend a new record cloning the information from eventMessageHandle, eventConfirmationCallback, userContextCallback.]*/
                if ((newEntry->messageHandle = IoTHubMessage_Clone(eventMessageHandle)) == NULL)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_014: [If cloning and/or adding the information fails for any reason, IoTHubClient_LL_SendEventAsync shall fail and return IOTHUB_CLIENT_ERROR.] */
                    result = IOTHUB_CLIENT_ERROR;
                    free(newEntry);
                    LOG_ERROR_RESULT;
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_013: [IoTHubClient_LL_SendEventAsync shall add the DLIST waitingToSend a new record cloning the information from eventMessageHandle, eventConfirmationCallback, userContextCallback.]*/
                    newEntry->callback = eventConfirmationCallback;
                    newEntry->context = userContextCallback;
                    DList_InsertTailList(&(iotHubClientHandle->waitingToSend), &(newEntry->entry));
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_015: [Otherwise IoTHubClient_LL_SendEventAsync shall succeed and return IOTHUB_CLIENT_OK.] */
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetMessageCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle == NULL)
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_02_016: [IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL.] */
        LogError("Invalid argument - iotHubClientHandle is NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        if (messageCallback == NULL)
        {
            if (handleData->messageCallback.type == CALLBACK_TYPE_NONE)
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_10_010: [If parameter messageCallback is NULL and the _SetMessageCallback had not been called to subscribe for messages, then IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_ERROR.] */
                LogError("not currently set to accept or process incoming messages.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if (handleData->messageCallback.type == CALLBACK_TYPE_ASYNC)
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_10_010: [If parameter messageCallback is NULL and the _SetMessageCallback had not been called to subscribe for messages, then IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_ERROR.] */
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClient_LL_SetMessageCallback_Ex function.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_02_019: [If parameter messageCallback is NULL then IoTHubClient_LL_SetMessageCallback shall call the underlying layer's _Unsubscribe function and return IOTHUB_CLIENT_OK.] */
                handleData->IoTHubTransport_Unsubscribe(handleData->deviceHandle);
                handleData->messageCallback.type = CALLBACK_TYPE_NONE;
                handleData->messageCallback.callbackSync = NULL;
                handleData->messageCallback.callbackAsync = NULL;
                handleData->messageCallback.userContextCallback = NULL;
                result = IOTHUB_CLIENT_OK;
            }
        }
        else
        {
            if (handleData->messageCallback.type == CALLBACK_TYPE_ASYNC)
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_10_011: [If parameter messageCallback is non-NULL and the _SetMessageCallback_Ex had been used to susbscribe for messages, then IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_ERROR.] */
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClient_LL_SetMessageCallback_Ex function before subscribing with MessageCallback.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                if (handleData->IoTHubTransport_Subscribe(handleData->deviceHandle) == 0)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_017: [If parameter messageCallback is non-NULL then IoTHubClient_LL_SetMessageCallback shall call the underlying layer's _Subscribe function.]*/
                    handleData->messageCallback.type = CALLBACK_TYPE_SYNC;
                    handleData->messageCallback.callbackSync = messageCallback;
                    handleData->messageCallback.userContextCallback = userContextCallback;
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_02_018: [If the underlying layer's _Subscribe function fails, then IoTHubClient_LL_SetMessageCallback shall fail and return IOTHUB_CLIENT_ERROR. Otherwise IoTHubClient_LL_SetMessageCallback shall succeed and return IOTHUB_CLIENT_OK.]*/
                    LogError("IoTHubTransport_Subscribe failed");
                    handleData->messageCallback.type = CALLBACK_TYPE_NONE;
                    handleData->messageCallback.callbackSync = NULL;
                    handleData->messageCallback.callbackAsync = NULL;
                    handleData->messageCallback.userContextCallback = NULL;
                    result = IOTHUB_CLIENT_ERROR;
                }
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetMessageCallback_Ex(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC_EX messageCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    if (iotHubClientHandle == NULL)
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_10_021: [IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL.]*/
        LogError("Invalid argument - iotHubClientHandle is NULL");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        if (messageCallback == NULL)
        {
            if (handleData->messageCallback.type == CALLBACK_TYPE_NONE)
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_10_018: [If parameter messageCallback is NULL and IoTHubClient_LL_SetMessageCallback_Ex had not been used to subscribe for messages, then IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR.] */
                LogError("not currently set to accept or process incoming messages.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if (handleData->messageCallback.type == CALLBACK_TYPE_SYNC)
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_10_019: [If parameter messageCallback is NULL and IoTHubClient_LL_SetMessageCallback had been used to subscribe for messages, then IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR.] */
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClient_LL_SetMessageCallback function.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_10_023: [If parameter messageCallback is NULL then IoTHubClient_LL_SetMessageCallback_Ex shall call the underlying layer's _Unsubscribe function and return IOTHUB_CLIENT_OK.] */ 
                handleData->IoTHubTransport_Unsubscribe(handleData->deviceHandle);
                handleData->messageCallback.type = CALLBACK_TYPE_NONE;
                handleData->messageCallback.callbackSync = NULL;
                handleData->messageCallback.callbackAsync = NULL;
                handleData->messageCallback.userContextCallback = NULL;
                result = IOTHUB_CLIENT_OK;
            }
        }
        else
        {
            if (handleData->messageCallback.type == CALLBACK_TYPE_SYNC)
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_10_020: [If parameter messageCallback is non-NULL, and IoTHubClient_LL_SetMessageCallback had been used to subscribe for messages, then IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR.] */
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClient_LL_MessageCallbackEx function before subscribing with MessageCallback.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                if (handleData->IoTHubTransport_Subscribe(handleData->deviceHandle) == 0)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_10_024: [If parameter messageCallback is non-NULL then IoTHubClient_LL_SetMessageCallback_Ex shall call the underlying layer's _Subscribe function.]*/
                    handleData->messageCallback.type = CALLBACK_TYPE_ASYNC;
                    handleData->messageCallback.callbackAsync = messageCallback;
                    handleData->messageCallback.userContextCallback = userContextCallback;
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_10_025: [If the underlying layer's _Subscribe function fails, then IoTHubClient_LL_SetMessageCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR. Otherwise IoTHubClient_LL_SetMessageCallback_Ex shall succeed and return IOTHUB_CLIENT_OK.] */
                    LogError("IoTHubTransport_Subscribe failed");
                    handleData->messageCallback.type = CALLBACK_TYPE_NONE;
                    handleData->messageCallback.callbackSync = NULL;
                    handleData->messageCallback.callbackAsync = NULL;
                    handleData->messageCallback.userContextCallback = NULL;
                    result = IOTHUB_CLIENT_ERROR;
                }
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendMessageDisposition(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, MESSAGE_CALLBACK_INFO* message_data, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    IOTHUB_CLIENT_RESULT result;
    if ((iotHubClientHandle == NULL) || (message_data == NULL))
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_10_026: [IoTHubClient_LL_SendMessageDisposition shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL.]*/
        LogError("Invalid argument handle=%p, message=%p", iotHubClientHandle, message_data);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        /*Codes_SRS_IOTHUBCLIENT_LL_10_027: [IoTHubClient_LL_SendMessageDisposition shall return the result from calling the underlying layer's _Send_Message_Disposition.]*/
        result = handleData->IoTHubTransport_SendMessageDisposition(message_data, disposition);
    }
    return result;
}

static void DoTimeouts(IOTHUB_CLIENT_LL_HANDLE_DATA* handleData)
{
    tickcounter_ms_t nowTick;
    if (tickcounter_get_current_ms(handleData->tickCounter, &nowTick) != 0)
    {
        LogError("unable to get the current ms, timeouts will not be processed");
    }
    else
    {
        DLIST_ENTRY* currentItemInWaitingToSend = handleData->waitingToSend.Flink;
        while (currentItemInWaitingToSend != &(handleData->waitingToSend)) /*while we are not at the end of the list*/
        {
            IOTHUB_MESSAGE_LIST* fullEntry = containingRecord(currentItemInWaitingToSend, IOTHUB_MESSAGE_LIST, entry);
            /*Codes_SRS_IOTHUBCLIENT_LL_02_041: [ If more than value miliseconds have passed since the call to IoTHubClient_LL_SendEventAsync then the message callback shall be called with a status code of IOTHUB_CLIENT_CONFIRMATION_TIMEOUT. ]*/
            if ((fullEntry->ms_timesOutAfter != 0) && (fullEntry->ms_timesOutAfter < nowTick))
            {
                PDLIST_ENTRY theNext = currentItemInWaitingToSend->Flink; /*need to save the next item, because the below operations are destructive*/
                DList_RemoveEntryList(currentItemInWaitingToSend);
                if (fullEntry->callback != NULL)
                {
                    fullEntry->callback(IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT, fullEntry->context);
                }
                IoTHubMessage_Destroy(fullEntry->messageHandle); /*because it has been cloned*/
                free(fullEntry);
                currentItemInWaitingToSend = theNext;
            }
            else
            {
                currentItemInWaitingToSend = currentItemInWaitingToSend->Flink;
            }
        }
    }
}

void IoTHubClient_LL_DoWork(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
{
    /*Codes_SRS_IOTHUBCLIENT_LL_02_020: [If parameter iotHubClientHandle is NULL then IoTHubClient_LL_DoWork shall not perform any action.] */
    if (iotHubClientHandle != NULL)
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        DoTimeouts(handleData);

        /*Codes_SRS_IOTHUBCLIENT_LL_07_008: [ IoTHubClient_LL_DoWork shall iterate the message queue and execute the underlying transports IoTHubTransport_ProcessItem function for each item. ] */
        DLIST_ENTRY* client_item = handleData->iot_msg_queue.Flink;
        while (client_item != &(handleData->iot_msg_queue)) /*while we are not at the end of the list*/
        {
            PDLIST_ENTRY next_item = client_item->Flink;

            IOTHUB_DEVICE_TWIN* queue_data = containingRecord(client_item, IOTHUB_DEVICE_TWIN, entry);
            IOTHUB_IDENTITY_INFO identity_info;
            identity_info.device_twin = queue_data;
            IOTHUB_PROCESS_ITEM_RESULT process_results =  handleData->IoTHubTransport_ProcessItem(handleData->transportHandle, IOTHUB_TYPE_DEVICE_TWIN, &identity_info);
            if (process_results == IOTHUB_PROCESS_CONTINUE || process_results == IOTHUB_PROCESS_NOT_CONNECTED)
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_07_010: [ If 'IoTHubTransport_ProcessItem' returns IOTHUB_PROCESS_CONTINUE or IOTHUB_PROCESS_NOT_CONNECTED IoTHubClient_LL_DoWork shall continue on to call the underlaying layer's _DoWork function. ]*/
                break;
            }
            else 
            {
                DList_RemoveEntryList(client_item);
                if (process_results == IOTHUB_PROCESS_OK)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_07_011: [ If 'IoTHubTransport_ProcessItem' returns IOTHUB_PROCESS_OK IoTHubClient_LL_DoWork shall add the IOTHUB_DEVICE_TWIN to the ack queue. ]*/
                    DList_InsertTailList(&(iotHubClientHandle->iot_ack_queue), &(queue_data->entry));
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_07_012: [ If 'IoTHubTransport_ProcessItem' returns any other value IoTHubClient_LL_DoWork shall destroy the IOTHUB_DEVICE_TWIN item. ]*/
                    LogError("Failure queue processing item");
                    device_twin_data_destroy(queue_data);
                }
            }
            // Move along to the next item
            client_item = next_item;
        }

        /*Codes_SRS_IOTHUBCLIENT_LL_02_021: [Otherwise, IoTHubClient_LL_DoWork shall invoke the underlaying layer's _DoWork function.]*/
        handleData->IoTHubTransport_DoWork(handleData->transportHandle, iotHubClientHandle);
    }
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetSendStatus(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    IOTHUB_CLIENT_RESULT result;

    /* Codes_SRS_IOTHUBCLIENT_09_007: [IoTHubClient_GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter] */
    if (iotHubClientHandle == NULL || iotHubClientStatus == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;

        /* Codes_SRS_IOTHUBCLIENT_09_008: [IoTHubClient_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_IDLE if there is currently no items to be sent] */
        /* Codes_SRS_IOTHUBCLIENT_09_009: [IoTHubClient_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_BUSY if there are currently items to be sent] */
        result = handleData->IoTHubTransport_GetSendStatus(handleData->deviceHandle, iotHubClientStatus);
    }

    return result;
}

void IoTHubClient_LL_SendComplete(IOTHUB_CLIENT_LL_HANDLE handle, PDLIST_ENTRY completed, IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
    /*Codes_SRS_IOTHUBCLIENT_LL_02_022: [If parameter completed is NULL, or parameter handle is NULL then IoTHubClient_LL_SendBatch shall return.]*/
    if (
        (handle == NULL) ||
        (completed == NULL)
        )
    {
        /*"shall return"*/
        LogError("invalid arg");
    }
    else
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_02_027: [If parameter result is IOTHUB_CLIENT_CONFIRMATION_ERROR then IoTHubClient_LL_SendComplete shall call all the non-NULL callbacks with the result parameter set to IOTHUB_CLIENT_CONFIRMATION_ERROR and the context set to the context passed originally in the SendEventAsync call.] */
        /*Codes_SRS_IOTHUBCLIENT_LL_02_025: [If parameter result is IOTHUB_CLIENT_CONFIRMATION_OK then IoTHubClient_LL_SendComplete shall call all the non-NULL callbacks with the result parameter set to IOTHUB_CLIENT_CONFIRMATION_OK and the context set to the context passed originally in the SendEventAsync call.]*/
        PDLIST_ENTRY oldest;
        while ((oldest = DList_RemoveHeadList(completed)) != completed)
        {
            IOTHUB_MESSAGE_LIST* messageList = (IOTHUB_MESSAGE_LIST*)containingRecord(oldest, IOTHUB_MESSAGE_LIST, entry);
            /*Codes_SRS_IOTHUBCLIENT_LL_02_026: [If any callback is NULL then there shall not be a callback call.]*/
            if (messageList->callback != NULL)
            {
                messageList->callback(result, messageList->context);
            }
            IoTHubMessage_Destroy(messageList->messageHandle);
            free(messageList);
        }
    }
}

int IoTHubClient_LL_DeviceMethodComplete(IOTHUB_CLIENT_LL_HANDLE handle, const char* method_name, const unsigned char* payLoad, size_t size, METHOD_HANDLE response_id)
{
    int result;
    if (handle == NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_LL_07_017: [ If handle or response is NULL then IoTHubClient_LL_DeviceMethodComplete shall return 500. ] */
        LogError("Invalid argument handle=%p", handle);
        result = __FAILURE__;
    }
    else
    {
        /* Codes_SRS_IOTHUBCLIENT_LL_07_018: [ If deviceMethodCallback is not NULL IoTHubClient_LL_DeviceMethodComplete shall execute deviceMethodCallback and return the status. ] */
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)handle;
        switch (handleData->methodCallback.type)
        {
            case CALLBACK_TYPE_SYNC:
            {
                unsigned char* payload_resp = NULL;
                size_t response_size = 0;
                result = handleData->methodCallback.callbackSync(method_name, payLoad, size, &payload_resp, &response_size, handleData->methodCallback.userContextCallback);
                /* Codes_SRS_IOTHUBCLIENT_LL_07_020: [ deviceMethodCallback shall build the BUFFER_HANDLE with the response payload from the IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC callback. ] */
                if (payload_resp != NULL && response_size > 0)
                {
                    result = handleData->IoTHubTransport_DeviceMethod_Response(handleData->deviceHandle, response_id, payload_resp, response_size, result);
                }
                else
                {
                    result = __FAILURE__;
                }
                if (payload_resp != NULL)
                {
                    free(payload_resp);
                }
                break;
            }
            case CALLBACK_TYPE_ASYNC:
                result = handleData->methodCallback.callbackAsync(method_name, payLoad, size, response_id, handleData->methodCallback.userContextCallback);
                break;
            default:
                /* Codes_SRS_IOTHUBCLIENT_LL_07_019: [ If deviceMethodCallback is NULL IoTHubClient_LL_DeviceMethodComplete shall return 404. ] */
                result = 0;
                break;
        }
    }
    return result;
}

void IoTHubClient_LL_RetrievePropertyComplete(IOTHUB_CLIENT_LL_HANDLE handle, DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size)
{
    if (handle == NULL)
    {
        /* Codes_SRS_IOTHUBCLIENT_LL_07_013: [ If handle is NULL then IoTHubClient_LL_RetrievePropertyComplete shall do nothing.] */
        LogError("Invalid argument handle=%p", handle);
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)handle;
        /* Codes_SRS_IOTHUBCLIENT_LL_07_014: [ If deviceTwinCallback is NULL then IoTHubClient_LL_RetrievePropertyComplete shall do nothing.] */
        if (handleData->deviceTwinCallback)
        {
            /* Codes_SRS_IOTHUBCLIENT_LL_07_015: [ If the the update_state parameter is DEVICE_TWIN_UPDATE_PARTIAL and a DEVICE_TWIN_UPDATE_COMPLETE has not been previously recieved then IoTHubClient_LL_RetrievePropertyComplete shall do nothing.] */
            if (update_state == DEVICE_TWIN_UPDATE_COMPLETE)
            {
                handleData->complete_twin_update_encountered = true;
            }
            if (handleData->complete_twin_update_encountered)
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_07_016: [ If deviceTwinCallback is set and DEVICE_TWIN_UPDATE_COMPLETE has been encountered then IoTHubClient_LL_RetrievePropertyComplete shall call deviceTwinCallback.] */
                handleData->deviceTwinCallback(update_state, payLoad, size, handleData->deviceTwinContextCallback);
            }
        }
    }
}

void IoTHubClient_LL_ReportedStateComplete(IOTHUB_CLIENT_LL_HANDLE handle, uint32_t item_id, int status_code)
{
    /* Codes_SRS_IOTHUBCLIENT_LL_07_002: [ if handle or queue_handle are NULL then IoTHubClient_LL_ReportedStateComplete shall do nothing. ] */
    if (handle == NULL)
    {
        /*"shall return"*/
        LogError("Invalid argument handle=%p", handle);
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)handle;

        /* Codes_SRS_IOTHUBCLIENT_LL_07_003: [ IoTHubClient_LL_ReportedStateComplete shall enumerate through the IOTHUB_DEVICE_TWIN structures in queue_handle. ]*/
        DLIST_ENTRY* client_item = handleData->iot_ack_queue.Flink;
        while (client_item != &(handleData->iot_ack_queue)) /*while we are not at the end of the list*/
        {
            PDLIST_ENTRY next_item = client_item->Flink;
            IOTHUB_DEVICE_TWIN* queue_data = containingRecord(client_item, IOTHUB_DEVICE_TWIN, entry);
            if (queue_data->item_id == item_id)
            {
                if (queue_data->reported_state_callback != NULL)
                {
                    queue_data->reported_state_callback(status_code, queue_data->context);
                }
                /*Codes_SRS_IOTHUBCLIENT_LL_07_009: [ IoTHubClient_LL_ReportedStateComplete shall remove the IOTHUB_DEVICE_TWIN item from the ack queue.]*/
                DList_RemoveEntryList(client_item);
                device_twin_data_destroy(queue_data);
                break;
            }
            client_item = next_item;
        }
    }
}

bool IoTHubClient_LL_MessageCallback(IOTHUB_CLIENT_LL_HANDLE handle, MESSAGE_CALLBACK_INFO* messageData)
{
    bool result;
    if ((handle == NULL) || messageData == NULL)
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_02_029: [If parameter handle is NULL then IoTHubClient_LL_MessageCallback shall return IOTHUBMESSAGE_ABANDONED.] */
        LogError("invalid argument: handle(%p), messageData(%p)", handle, messageData);
        result = false;
    }
    else if (messageData->messageHandle == NULL)
    {
        /*Codes_SRS_IOTHUBCLIENT_LL_10_004: [If messageHandle field of paramger messageData is NULL then IoTHubClient_LL_MessageCallback shall return IOTHUBMESSAGE_ABANDONED.] */
        LogError("invalid argument messageData->messageHandle(NULL)");
        result = false;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)handle;

        /* Codes_SRS_IOTHUBCLIENT_LL_09_004: [IoTHubClient_LL_GetLastMessageReceiveTime shall return lastMessageReceiveTime in localtime] */
        handleData->lastMessageReceiveTime = get_time(NULL);
        switch (handleData->messageCallback.type)
        {
            case CALLBACK_TYPE_NONE:
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_02_032: [If the client is not subscribed to receive messages then IoTHubClient_LL_MessageCallback shall return false.] */
                LogError("Invalid workflow - not currently set up to accept messages");
                result = false;
                break;
            }
            case CALLBACK_TYPE_SYNC:
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_02_030: [If messageCallbackType is LEGACY then IoTHubClient_LL_MessageCallback shall invoke the last callback function (the parameter messageCallback to IoTHubClient_LL_SetMessageCallback) passing the message and the passed userContextCallback.]*/
                IOTHUBMESSAGE_DISPOSITION_RESULT cb_result = handleData->messageCallback.callbackSync(messageData->messageHandle, handleData->messageCallback.userContextCallback);

                /*Codes_SRS_IOTHUBCLIENT_LL_10_007: [If messageCallbackType is LEGACY then IoTHubClient_LL_MessageCallback shall send the message disposition as returned by the client to the underlying layer.] */
                if (handleData->IoTHubTransport_SendMessageDisposition(messageData, cb_result) != IOTHUB_CLIENT_OK)
                {
                    LogError("IoTHubTransport_SendMessageDisposition failed");
                }
                result = true;
                break;
            }
            case CALLBACK_TYPE_ASYNC:
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_10_009: [If messageCallbackType is ASYNC then IoTHubClient_LL_MessageCallback shall return what messageCallbacEx returns.] */
                result = handleData->messageCallback.callbackAsync(messageData, handleData->messageCallback.userContextCallback);
                if (!result)
                {
                    LogError("messageCallbackEx failed");
                }
                break;
            }
            default:
            {
                LogError("Invalid state");
                result = false;
                break;
            }
        }
    }
    return result;
}

void IoTHubClient_LL_ConnectionStatusCallBack(IOTHUB_CLIENT_LL_HANDLE handle, IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    /*Codes_SRS_IOTHUBCLIENT_LL_25_113: [If parameter connectionStatus is NULL or parameter handle is NULL then IoTHubClient_LL_ConnectionStatusCallBack shall return.]*/
    if (handle == NULL)
    {
        /*"shall return"*/
        LogError("invalid arg");
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)handle;

        /*Codes_SRS_IOTHUBCLIENT_LL_25_114: [IoTHubClient_LL_ConnectionStatusCallBack shall call non-callback set by the user from IoTHubClient_LL_SetConnectionStatusCallback passing the status, reason and the passed userContextCallback.]*/
        if (handleData->conStatusCallback != NULL)
        {
            handleData->conStatusCallback(status, reason, handleData->conStatusUserContextCallback);
        }
    }

}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetConnectionStatusCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback, void * userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    /*Codes_SRS_IOTHUBCLIENT_LL_25_111: [IoTHubClient_LL_SetConnectionStatusCallback shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter iotHubClientHandle**]** */
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        /*Codes_SRS_IOTHUBCLIENT_LL_25_112: [IoTHubClient_LL_SetConnectionStatusCallback shall return IOTHUB_CLIENT_OK and save the callback and userContext as a member of the handle.] */
        handleData->conStatusCallback = connectionStatusCallback;
        handleData->conStatusUserContextCallback = userContextCallback;
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;

    /* Codes_SRS_IOTHUBCLIENT_LL_25_116: [If iotHubClientHandle, retryPolicy or retryTimeoutLimitinSeconds is NULL, IoTHubClient_LL_GetRetryPolicy shall return IOTHUB_CLIENT_INVALID_ARG]*/
    if (handleData == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        if (handleData->transportHandle == NULL)
        {
            result = IOTHUB_CLIENT_ERROR;
            LOG_ERROR_RESULT;
        }
        else
        {
            if (handleData->IoTHubTransport_SetRetryPolicy(handleData->transportHandle, retryPolicy, retryTimeoutLimitInSeconds) != 0)
            {
                result = IOTHUB_CLIENT_ERROR;
                LOG_ERROR_RESULT;
            }
            else
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_25_118: [IoTHubClient_LL_SetRetryPolicy shall save connection retry policies specified by the user to retryPolicy in struct IOTHUB_CLIENT_LL_HANDLE_DATA] */
                /* Codes_SRS_IOTHUBCLIENT_LL_25_119: [IoTHubClient_LL_SetRetryPolicy shall save retryTimeoutLimitInSeconds in seconds to retryTimeout in struct IOTHUB_CLIENT_LL_HANDLE_DATA] */
                handleData->retryPolicy = retryPolicy;
                handleData->retryTimeoutLimitInSeconds = retryTimeoutLimitInSeconds;
                result = IOTHUB_CLIENT_OK;
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetRetryPolicy(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy, size_t* retryTimeoutLimitInSeconds)
{
    IOTHUB_CLIENT_RESULT result;

    /* Codes_SRS_IOTHUBCLIENT_LL_09_001: [IoTHubClient_LL_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_INVALID_ARG if any of the arguments is NULL] */
    if (iotHubClientHandle == NULL || retryPolicy == NULL || retryTimeoutLimitInSeconds == NULL)
    {
        LogError("Invalid parameter IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = %p, IOTHUB_CLIENT_RETRY_POLICY* retryPolicy = %p, size_t* retryTimeoutLimitInSeconds = %p", iotHubClientHandle, retryPolicy, retryTimeoutLimitInSeconds);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;

        *retryPolicy = handleData->retryPolicy;
        *retryTimeoutLimitInSeconds = handleData->retryTimeoutLimitInSeconds;
        result = IOTHUB_CLIENT_OK;
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetLastMessageReceiveTime(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, time_t* lastMessageReceiveTime)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;

    /* Codes_SRS_IOTHUBCLIENT_LL_09_001: [IoTHubClient_LL_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_INVALID_ARG if any of the arguments is NULL] */
    if (handleData == NULL || lastMessageReceiveTime == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        /* Codes_SRS_IOTHUBCLIENT_LL_09_002: [IoTHubClient_LL_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_INDEFINITE_TIME - and not set 'lastMessageReceiveTime' - if it is unable to provide the time for the last commands] */
        if (handleData->lastMessageReceiveTime == INDEFINITE_TIME)
        {
            result = IOTHUB_CLIENT_INDEFINITE_TIME;
            LOG_ERROR_RESULT;
        }
        else
        {
            /* Codes_SRS_IOTHUBCLIENT_LL_09_003: [IoTHubClient_LL_GetLastMessageReceiveTime shall return IOTHUB_CLIENT_OK if it wrote in the lastMessageReceiveTime the time when the last command was received] */
            /* Codes_SRS_IOTHUBCLIENT_LL_09_004: [IoTHubClient_LL_GetLastMessageReceiveTime shall return lastMessageReceiveTime in localtime] */
            *lastMessageReceiveTime = handleData->lastMessageReceiveTime;
            result = IOTHUB_CLIENT_OK;
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetOption(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, const void* value)
{

    IOTHUB_CLIENT_RESULT result;
    /*Codes_SRS_IOTHUBCLIENT_LL_02_034: [If iotHubClientHandle is NULL then IoTHubClient_LL_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.]*/
    /*Codes_SRS_IOTHUBCLIENT_LL_02_035: [If optionName is NULL then IoTHubClient_LL_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
    /*Codes_SRS_IOTHUBCLIENT_LL_02_036: [If value is NULL then IoTHubClient_LL_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
    if (
        (iotHubClientHandle == NULL) ||
        (optionName == NULL) ||
        (value == NULL)
        )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid argument (NULL)");
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;

        /*Codes_SRS_IOTHUBCLIENT_LL_02_039: [ "messageTimeout" - once IoTHubClient_LL_SendEventAsync is called the message shall timeout after value miliseconds. Value is a pointer to a uint64. ]*/
        if (strcmp(optionName, OPTION_MESSAGE_TIMEOUT) == 0)
        {
            /*this is an option handled by IoTHubClient_LL*/
            /*Codes_SRS_IOTHUBCLIENT_LL_02_043: [ Calling IoTHubClient_LL_SetOption with value set to "0" shall disable the timeout mechanism for all new messages. ]*/
            handleData->currentMessageTimeout = *(const tickcounter_ms_t*)value;
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(optionName, OPTION_PRODUCT_INFO) == 0)
        {
            /*Codes_SRS_IOTHUBCLIENT_LL_10_033: [repeat calls with "product_info" will erase the previously set product information if applicatble. ]*/
            if (handleData->product_info != NULL)
            {
                STRING_delete(handleData->product_info);
                handleData->product_info = NULL;
            }

            /*Codes_SRS_IOTHUBCLIENT_LL_10_035: [If string concatenation fails, `IoTHubClient_LL_SetOption` shall return `IOTHUB_CLIENT_ERRROR`. Otherwise, `IOTHUB_CLIENT_OK` shall be returned. ]*/
            handleData->product_info = make_product_info((const char*)value);
            if (handleData->product_info == NULL)
            {
                LogError("STRING_construct_sprintf failed");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                result = IOTHUB_CLIENT_OK;
            }
        }
        else
        {

            /*Codes_SRS_IOTHUBCLIENT_LL_02_099: [ IoTHubClient_LL_SetOption shall return according to the table below ]*/
            IOTHUB_CLIENT_RESULT uploadToBlob_result; 
#ifdef USE_UPLOADTOBLOB
            uploadToBlob_result = IoTHubClient_LL_UploadToBlob_SetOption(handleData->uploadToBlobHandle, optionName, value);
            if(uploadToBlob_result == IOTHUB_CLIENT_ERROR)
            {
                LogError("unable to IoTHubClient_LL_UploadToBlob_SetOption");
                result = IOTHUB_CLIENT_ERROR;
            }
#else
            uploadToBlob_result = IOTHUB_CLIENT_INVALID_ARG; /*harmless value (IOTHUB_CLIENT_INVALID_ARG)in the case when uploadtoblob is not compiled in, otherwise whatever IoTHubClient_LL_UploadToBlob_SetOption returned*/
#endif /*USE_UPLOADTOBLOB*/

            /*Codes_SRS_IOTHUBCLIENT_LL_12_023: [** `c2d_keep_alive_freq_secs` - shall set the cloud to device keep alive frequency(in seconds) for the connection. Zero means keep alive will not be sent. ]*/
            result =
                /*based on uploadToBlob_result value this is what happens:*/
                /*IOTHUB_CLIENT_INVALID_ARG always returns what IoTHubTransport_SetOption returns*/
                /*IOTHUB_CLIENT_ERROR always returns IOTHUB_CLIENT_ERROR */
                /*IOTHUB_CLIENT_OK returns OK
                    IOTHUB_CLIENT_OK if IoTHubTransport_SetOption returns OK or INVALID_ARG
                    IOTHUB_CLIENT_ERROR if IoTHubTransport_SetOption returns ERROR*/

                (uploadToBlob_result == IOTHUB_CLIENT_INVALID_ARG) ? handleData->IoTHubTransport_SetOption(handleData->transportHandle, optionName, value) :
                (uploadToBlob_result == IOTHUB_CLIENT_ERROR) ? IOTHUB_CLIENT_ERROR :
                (handleData->IoTHubTransport_SetOption(handleData->transportHandle, optionName, value) == IOTHUB_CLIENT_ERROR) ? IOTHUB_CLIENT_ERROR : IOTHUB_CLIENT_OK;

            if (result != IOTHUB_CLIENT_OK)
            {
                LogError("underlying transport failed, returned = %s", ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, result));
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_GetOption(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* optionName, void** value)
{
    IOTHUB_CLIENT_RESULT result;

    if ((iotHubClientHandle == NULL) || (optionName == NULL) || (value == NULL))
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid argument iotHubClientHandle(%p); optionName(%p); value(%p)", iotHubClientHandle, optionName, value);
    }
    else if (strcmp(optionName, OPTION_PRODUCT_INFO) == 0)
    {
        result = IOTHUB_CLIENT_OK;
        *value = iotHubClientHandle->product_info;
    }
    else
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid argument (%s)", optionName);
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceTwinCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    /* Codes_SRS_IOTHUBCLIENT_LL_10_001: [ IoTHubClient_LL_SetDeviceTwinCallback shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL.] */
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("Invalid argument specified iothubClientHandle=%p", iotHubClientHandle);
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        if (deviceTwinCallback == NULL)
        {
            /* Codes_SRS_IOTHUBCLIENT_LL_10_006: [ If deviceTwinCallback is NULL, then IoTHubClient_LL_SetDeviceTwinCallback shall call the underlying layer's _Unsubscribe function and return IOTHUB_CLIENT_OK.] */
            handleData->IoTHubTransport_Unsubscribe_DeviceTwin(handleData->transportHandle);
            handleData->deviceTwinCallback = NULL;
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            /* Codes_SRS_IOTHUBCLIENT_LL_10_002: [ If deviceTwinCallback is not NULL, then IoTHubClient_LL_SetDeviceTwinCallback shall call the underlying layer's _Subscribe function.] */
            if (handleData->IoTHubTransport_Subscribe_DeviceTwin(handleData->transportHandle) == 0)
            {
                handleData->deviceTwinCallback = deviceTwinCallback;
                handleData->deviceTwinContextCallback = userContextCallback;
                /* Codes_SRS_IOTHUBCLIENT_LL_10_005: [ Otherwise IoTHubClient_LL_SetDeviceTwinCallback shall succeed and return IOTHUB_CLIENT_OK.] */
                result = IOTHUB_CLIENT_OK;
            }
            else
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_10_003: [ If the underlying layer's _Subscribe function fails, then IoTHubClient_LL_SetDeviceTwinCallback shall fail and return IOTHUB_CLIENT_ERROR.] */
                result = IOTHUB_CLIENT_ERROR;
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SendReportedState(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* reportedState, size_t size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    /* Codes_SRS_IOTHUBCLIENT_LL_10_012: [ IoTHubClient_LL_SendReportedState shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL. ] */
    /* Codes_SRS_IOTHUBCLIENT_LL_10_013: [ IoTHubClient_LL_SendReportedState shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter reportedState is NULL] */
    /* Codes_SRS_IOTHUBCLIENT_LL_07_005: [ IoTHubClient_LL_SendReportedState shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter size is equal to 0. ] */
    if (iotHubClientHandle == NULL || (reportedState == NULL || size == 0) )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("Invalid argument specified iothubClientHandle=%p, reportedState=%p, size=%lu", iotHubClientHandle, reportedState, size);
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        /* Codes_SRS_IOTHUBCLIENT_LL_10_014: [IoTHubClient_LL_SendReportedState shall construct and queue the reported a Device_Twin structure for transmition by the underlying transport.] */
        IOTHUB_DEVICE_TWIN* client_data = dev_twin_data_create(handleData, get_next_item_id(handleData), reportedState, size, reportedStateCallback, userContextCallback);
        if (client_data == NULL)
        {
            /* Codes_SRS_IOTHUBCLIENT_LL_10_015: [If any error is encountered IoTHubClient_LL_SendReportedState shall return IOTHUB_CLIENT_ERROR.] */
            LogError("Failure constructing device twin data");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            if (handleData->IoTHubTransport_Subscribe_DeviceTwin(handleData->transportHandle) != 0)
            {
                LogError("Failure adding device twin data to queue");
                device_twin_data_destroy(client_data);
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_07_001: [ IoTHubClient_LL_SendReportedState shall queue the constructed reportedState data to be consumed by the targeted transport. ] */
                DList_InsertTailList(&(iotHubClientHandle->iot_msg_queue), &(client_data->entry));

                /* Codes_SRS_IOTHUBCLIENT_LL_10_016: [ Otherwise IoTHubClient_LL_SendReportedState shall succeed and return IOTHUB_CLIENT_OK.] */
                result = IOTHUB_CLIENT_OK;
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceMethodCallback(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;

    /*Codes_SRS_IOTHUBCLIENT_LL_12_017: [ IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_INVALID_ARG if parameter iotHubClientHandle is NULL. ] */
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        if (deviceMethodCallback == NULL)
        {
            if (handleData->methodCallback.type == CALLBACK_TYPE_NONE)
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_10_029: [ If deviceMethodCallback is NULL and the client is not subscribed to receive method calls, IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ] */
                LogError("not currently set to accept or process incoming messages.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if (handleData->methodCallback.type == CALLBACK_TYPE_ASYNC)
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_10_028: [If the user has subscribed using IoTHubClient_LL_SetDeviceMethodCallback_Ex, IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ] */
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClient_LL_SetDeviceMethodCallback_Ex function.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_02_019: [If parameter messageCallback is NULL then IoTHubClient_LL_SetMessageCallback shall call the underlying layer's _Unsubscribe function and return IOTHUB_CLIENT_OK.] */
                /*Codes_SRS_IOTHUBCLIENT_LL_12_018: [If deviceMethodCallback is NULL, then IoTHubClient_LL_SetDeviceMethodCallback shall call the underlying layer's IoTHubTransport_Unsubscribe_DeviceMethod function and return IOTHUB_CLIENT_OK. ] */
                /*Codes_SRS_IOTHUBCLIENT_LL_12_022: [ Otherwise IoTHubClient_LL_SetDeviceMethodCallback shall succeed and return IOTHUB_CLIENT_OK. ]*/
                handleData->IoTHubTransport_Unsubscribe_DeviceMethod(handleData->transportHandle);
                handleData->methodCallback.type = CALLBACK_TYPE_NONE;
                handleData->methodCallback.callbackSync = NULL;
                handleData->methodCallback.userContextCallback = NULL;
                result = IOTHUB_CLIENT_OK;
            }
        }
        else
        {
            if (handleData->methodCallback.type == CALLBACK_TYPE_ASYNC)
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_10_028: [If the user has subscribed using IoTHubClient_LL_SetDeviceMethodCallback_Ex, IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ] */
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClient_LL_SetDeviceMethodCallback_Ex function before subscribing with IoTHubClient_LL_SetDeviceMethodCallback.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                /*Codes_SRS_IOTHUBCLIENT_LL_12_019: [ If deviceMethodCallback is not NULL, then IoTHubClient_LL_SetDeviceMethodCallback shall call the underlying layer's IoTHubTransport_Subscribe_DeviceMethod function. ]*/
                if (handleData->IoTHubTransport_Subscribe_DeviceMethod(handleData->deviceHandle) == 0)
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_12_022: [ Otherwise IoTHubClient_LL_SetDeviceMethodCallback shall succeed and return IOTHUB_CLIENT_OK. ]*/
                    handleData->methodCallback.type = CALLBACK_TYPE_SYNC;
                    handleData->methodCallback.callbackSync = deviceMethodCallback;
                    handleData->methodCallback.callbackAsync = NULL;
                    handleData->methodCallback.userContextCallback = userContextCallback;
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_12_020: [ If the underlying layer's IoTHubTransport_Subscribe_DeviceMethod function fails, then IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                    /*Codes_SRS_IOTHUBCLIENT_LL_12_021: [ If adding the information fails for any reason, IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ]*/
                    LogError("IoTHubTransport_Subscribe_DeviceMethod failed");
                    handleData->methodCallback.type = CALLBACK_TYPE_NONE;
                    handleData->methodCallback.callbackAsync = NULL;
                    handleData->methodCallback.callbackSync = NULL;
                    handleData->methodCallback.userContextCallback = NULL;
                    result = IOTHUB_CLIENT_ERROR;
                }
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_SetDeviceMethodCallback_Ex(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK inboundDeviceMethodCallback, void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result;
    /* Codes_SRS_IOTHUBCLIENT_LL_07_021: [ If handle is NULL then IoTHubClient_LL_SetDeviceMethodCallback_Ex shall return IOTHUB_CLIENT_INVALID_ARG.] */
    if (iotHubClientHandle == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        if (inboundDeviceMethodCallback == NULL)
        {
            if (handleData->methodCallback.type == CALLBACK_TYPE_NONE)
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_10_030: [ If deviceMethodCallback is NULL and the client is not subscribed to receive method calls, IoTHubClient_LL_SetDeviceMethodCallback shall fail and return IOTHUB_CLIENT_ERROR. ] */
                LogError("not currently set to accept or process incoming messages.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if (handleData->methodCallback.type == CALLBACK_TYPE_SYNC)
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_10_031: [If the user has subscribed using IoTHubClient_LL_SetDeviceMethodCallback, IoTHubClient_LL_SetDeviceMethodCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR. ] */
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClient_LL_SetDeviceMethodCallback function.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_07_022: [ If inboundDeviceMethodCallback is NULL then IoTHubClient_LL_SetDeviceMethodCallback_Ex shall call the underlying layer's IoTHubTransport_Unsubscribe_DeviceMethod function and return IOTHUB_CLIENT_OK.] */
                handleData->IoTHubTransport_Unsubscribe_DeviceMethod(handleData->transportHandle);
                handleData->methodCallback.type = CALLBACK_TYPE_NONE;
                handleData->methodCallback.callbackAsync = NULL;
                handleData->methodCallback.userContextCallback = NULL;
                result = IOTHUB_CLIENT_OK;
            }
        }
        else
        {
            if (handleData->methodCallback.type == CALLBACK_TYPE_SYNC)
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_10_031: [If the user has subscribed using IoTHubClient_LL_SetDeviceMethodCallback, IoTHubClient_LL_SetDeviceMethodCallback_Ex shall fail and return IOTHUB_CLIENT_ERROR. ] */
                LogError("Invalid workflow sequence. Please unsubscribe using the IoTHubClient_LL_SetDeviceMethodCallback function before subscribing with IoTHubClient_LL_SetDeviceMethodCallback_Ex.");
                result = IOTHUB_CLIENT_ERROR;
            }
            else
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_07_023: [ If inboundDeviceMethodCallback is non-NULL then IoTHubClient_LL_SetDeviceMethodCallback_Ex shall call the underlying layer's IoTHubTransport_Subscribe_DeviceMethod function.]*/
                if (handleData->IoTHubTransport_Subscribe_DeviceMethod(handleData->deviceHandle) == 0)
                {
                    handleData->methodCallback.type = CALLBACK_TYPE_ASYNC;
                    handleData->methodCallback.callbackAsync = inboundDeviceMethodCallback;
                    handleData->methodCallback.callbackSync = NULL;
                    handleData->methodCallback.userContextCallback = userContextCallback;
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
                    /* Codes_SRS_IOTHUBCLIENT_LL_07_025: [ If any error is encountered then IoTHubClient_LL_SetDeviceMethodCallback_Ex shall return IOTHUB_CLIENT_ERROR.] */
                    LogError("IoTHubTransport_Subscribe_DeviceMethod failed");
                    handleData->methodCallback.type = CALLBACK_TYPE_NONE;
                    handleData->methodCallback.callbackAsync = NULL;
                    handleData->methodCallback.callbackSync = NULL;
                    handleData->methodCallback.userContextCallback = NULL;
                    result = IOTHUB_CLIENT_ERROR;
                }
            }
        }
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubClient_LL_DeviceMethodResponse(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, METHOD_HANDLE methodId, const unsigned char* response, size_t response_size, int status_response)
{
    IOTHUB_CLIENT_RESULT result;
    /* Codes_SRS_IOTHUBCLIENT_LL_07_026: [ If handle or methodId is NULL then IoTHubClient_LL_DeviceMethodResponse shall return IOTHUB_CLIENT_INVALID_ARG.] */
    if (iotHubClientHandle == NULL || methodId == NULL)
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LOG_ERROR_RESULT;
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE_DATA* handleData = (IOTHUB_CLIENT_LL_HANDLE_DATA*)iotHubClientHandle;
        /* Codes_SRS_IOTHUBCLIENT_LL_07_027: [ IoTHubClient_LL_DeviceMethodResponse shall call the IoTHubTransport_DeviceMethod_Response transport function.] */
        if (handleData->IoTHubTransport_DeviceMethod_Response(handleData->deviceHandle, methodId, response, response_size, status_response) != 0)
        {
            LogError("IoTHubTransport_DeviceMethod_Response failed");
            result = IOTHUB_CLIENT_ERROR;
        }
        else
        {
            result = IOTHUB_CLIENT_OK;
        }
    }
    return result;
}

#ifdef USE_UPLOADTOBLOB
IOTHUB_CLIENT_RESULT IoTHubClient_LL_UploadToBlob(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const char* destinationFileName, const unsigned char* source, size_t size)
{
    IOTHUB_CLIENT_RESULT result;
    /*Codes_SRS_IOTHUBCLIENT_LL_02_061: [ If iotHubClientHandle is NULL then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    /*Codes_SRS_IOTHUBCLIENT_LL_02_062: [ If destinationFileName is NULL then IoTHubClient_LL_UploadToBlob shall fail and return IOTHUB_CLIENT_INVALID_ARG. ]*/
    if (
        (iotHubClientHandle == NULL) ||
        (destinationFileName == NULL) ||
        ((source == NULL) && (size >0))
        )
    {
        LogError("invalid parameters IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle=%p, const char* destinationFileName=%s, const unsigned char* source=%p, size_t size=%lu", iotHubClientHandle, destinationFileName, source, size);
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        result = IoTHubClient_LL_UploadToBlob_Impl(iotHubClientHandle->uploadToBlobHandle, destinationFileName, source, size);
    }
    return result;
}
#endif

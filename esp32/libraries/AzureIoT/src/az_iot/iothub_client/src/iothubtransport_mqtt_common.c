// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <ctype.h>
#include "az_iot/c-utility/inc/azure_c_shared_utility/optimize_size.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/gballoc.h"

#include "az_iot/c-utility/inc/azure_c_shared_utility/xlogging.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/strings.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/doublylinkedlist.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/crt_abstractions.h"

#include "../inc/iothub_client_ll.h"
#include "../inc/iothub_client_options.h"
#include "../inc/iothub_client_private.h"
#include "az_iot/umqtt/inc/azure_umqtt_c/mqtt_client.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/sastoken.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/tickcounter.h"

#include "az_iot/c-utility/inc/azure_c_shared_utility/tlsio.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/platform.h"

#include "az_iot/c-utility/inc/azure_c_shared_utility/string_tokenizer.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/shared_util_options.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/urlencode.h"
#include "../inc/iothub_client_version.h"
#include "../inc/iothub_client_retry_control.h"

#include "../inc/iothubtransport_mqtt_common.h"

#include <stdarg.h>
#include <stdio.h>

#include <limits.h>
#include <inttypes.h>

#define SAS_TOKEN_DEFAULT_LIFETIME          3600
#define SAS_REFRESH_MULTIPLIER              .8
#define EPOCH_TIME_T_VALUE                  0
#define DEFAULT_MQTT_KEEPALIVE              4*60 // 4 min
#define BUILD_CONFIG_USERNAME               24
#define SAS_TOKEN_DEFAULT_LEN               10
#define RESEND_TIMEOUT_VALUE_MIN            1*60
#define MAX_SEND_RECOUNT_LIMIT              2
#define DEFAULT_CONNECTION_INTERVAL         30
#define FAILED_CONN_BACKOFF_VALUE           5
#define STATUS_CODE_FAILURE_VALUE           500
#define STATUS_CODE_TIMEOUT_VALUE           408

#define DEFAULT_RETRY_POLICY                IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER
#define DEFAULT_RETRY_TIMEOUT_IN_SECONDS    0

static const char TOPIC_DEVICE_TWIN_PREFIX[] = "$iothub/twin";
static const char TOPIC_DEVICE_METHOD_PREFIX[] = "$iothub/methods";

static const char* TOPIC_GET_DESIRED_STATE = "$iothub/twin/res/#";
static const char* TOPIC_NOTIFICATION_STATE = "$iothub/twin/PATCH/properties/desired/#";

static const char* TOPIC_DEVICE_MSG = "devices/%s/messages/devicebound/#";
static const char* TOPIC_DEVICE_DEVICE = "devices/%s/messages/events/";

static const char* TOPIC_DEVICE_METHOD_SUBSCRIBE = "$iothub/methods/POST/#";
static const char* TOPIC_DEVICE_METHOD_RESPONSE = "$iothub/methods/res";

static const char* IOTHUB_API_VERSION = "2016-11-14";

static const char* PROPERTY_SEPARATOR = "&";
static const char* REPORTED_PROPERTIES_TOPIC = "$iothub/twin/PATCH/properties/reported/?$rid=%"PRIu16;
static const char* GET_PROPERTIES_TOPIC = "$iothub/twin/GET/?$rid=%"PRIu16;
static const char* DEVICE_METHOD_RESPONSE_TOPIC = "$iothub/methods/res/%d/?$rid=%s";

static const char* REQUEST_ID_PROPERTY = "?$rid=";

static const char* MESSAGE_ID_PROPERTY = "mid";
static const char* CORRELATION_ID_PROPERTY = "cid";
static const char* CONTENT_TYPE_PROPERTY = "ct";
static const char* CONTENT_ENCODING_PROPERTY = "ce";

#define UNSUBSCRIBE_FROM_TOPIC                  0x0000
#define SUBSCRIBE_GET_REPORTED_STATE_TOPIC      0x0001
#define SUBSCRIBE_NOTIFICATION_STATE_TOPIC      0x0002
#define SUBSCRIBE_TELEMETRY_TOPIC               0x0004
#define SUBSCRIBE_METHODS_TOPIC                 0x0008
#define SUBSCRIBE_DEVICE_METHOD_TOPIC           0x0010
#define SUBSCRIBE_TOPIC_COUNT                   4

DEFINE_ENUM_STRINGS(MQTT_CLIENT_EVENT_ERROR, MQTT_CLIENT_EVENT_ERROR_VALUES)

typedef struct SYSTEM_PROPERTY_INFO_TAG
{
    const char* propName;
    size_t propLength;
} SYSTEM_PROPERTY_INFO;

static SYSTEM_PROPERTY_INFO sysPropList[] = {
    { "%24.exp", 7 },
    { "%24.mid", 7 },
    { "%24.uid", 7 },
    { "%24.to", 6 },
    { "%24.cid", 7 },
    { "%24.ct", 6 },
    { "%24.ce", 6 },
    { "devices/", 8 },
    { "iothub-operation", 16 },
    { "iothub-ack", 10 }
};

typedef enum DEVICE_TWIN_MSG_TYPE_TAG
{
    REPORTED_STATE,
    RETRIEVE_PROPERTIES
} DEVICE_TWIN_MSG_TYPE;

typedef enum MQTT_TRANSPORT_CREDENTIAL_TYPE_TAG
{
    CREDENTIAL_NOT_BUILD,
    X509,
    SAS_TOKEN_FROM_USER,
    DEVICE_KEY,
} MQTT_TRANSPORT_CREDENTIAL_TYPE;

typedef enum MQTT_CLIENT_STATUS_TAG
{
    MQTT_CLIENT_STATUS_NOT_CONNECTED,
    MQTT_CLIENT_STATUS_CONNECTING,
    MQTT_CLIENT_STATUS_CONNECTED
} MQTT_CLIENT_STATUS;

typedef struct MQTTTRANSPORT_HANDLE_DATA_TAG
{
    // Topic control
    STRING_HANDLE topic_MqttEvent;
    STRING_HANDLE topic_MqttMessage;
    STRING_HANDLE topic_GetState;
    STRING_HANDLE topic_NotifyState;

    STRING_HANDLE topic_DeviceMethods;

    uint32_t topics_ToSubscribe;

    // Connection related constants
    STRING_HANDLE hostAddress;
    STRING_HANDLE device_id;
    STRING_HANDLE devicesPath;
    int portNum;

    MQTT_GET_IO_TRANSPORT get_io_transport;

    // The current mqtt iothub implementation requires that the hub name and the domain suffix be passed as the first of a series of segments
    // passed through the username portion of the connection frame.
    // The second segment will contain the device id.  The two segments are delemited by a "/".
    // The first segment can be a maximum 256 characters.
    // The second segment can be a maximum 128 characters.
    // With the / delimeter you have 384 chars (Plus a terminator of 0).
    STRING_HANDLE configPassedThroughUsername;

    // Upper layer
    IOTHUB_CLIENT_LL_HANDLE llClientHandle;

    // Protocol 
    MQTT_CLIENT_HANDLE mqttClient;
    XIO_HANDLE xioTransport;

    // Session - connection
    uint16_t packetId;

    // Connection state control
    bool isRegistered;
    MQTT_CLIENT_STATUS mqttClientStatus;
    bool isDestroyCalled;
    bool device_twin_get_sent;
    bool isRecoverableError;
    uint16_t keepAliveValue;
    tickcounter_ms_t mqtt_connect_time;
    size_t connectFailCount;
    tickcounter_ms_t connectTick;
    bool log_trace;
    bool raw_trace;
    TICK_COUNTER_HANDLE msgTickCounter;
    OPTIONHANDLER_HANDLE saved_tls_options;		// Here are the options from the xio layer if any is saved.

    // Internal lists for message tracking
    PDLIST_ENTRY waitingToSend;
    DLIST_ENTRY ack_waiting_queue;

    // Message tracking
    CONTROL_PACKET_TYPE currPacketState;

    // Telemetry specific
    DLIST_ENTRY telemetry_waitingForAck;

    // Controls frequency of reconnection logic.
    RETRY_CONTROL_HANDLE retry_control_handle;

    // Auth module used to generating handle authorization
    // with either SAS Token, x509 Certs, and Device SAS Token
    IOTHUB_AUTHORIZATION_HANDLE authorization_module;

    char* http_proxy_hostname;
    int http_proxy_port;
    char* http_proxy_username;
    char* http_proxy_password;
    bool isProductInfoSet;
} MQTTTRANSPORT_HANDLE_DATA, *PMQTTTRANSPORT_HANDLE_DATA;

typedef struct MQTT_DEVICE_TWIN_ITEM_TAG
{
    tickcounter_ms_t msgPublishTime;
    size_t retryCount;
    IOTHUB_IDENTITY_TYPE iothub_type;
    uint16_t packet_id;
    uint32_t iothub_msg_id;
    IOTHUB_DEVICE_TWIN* device_twin_data;
    DEVICE_TWIN_MSG_TYPE device_twin_msg_type;
    DLIST_ENTRY entry;
} MQTT_DEVICE_TWIN_ITEM;

typedef struct MQTT_MESSAGE_DETAILS_LIST_TAG
{
    tickcounter_ms_t msgPublishTime;
    size_t retryCount;
    IOTHUB_MESSAGE_LIST* iotHubMessageEntry;
    void* context;
    uint16_t packet_id;
    DLIST_ENTRY entry;
} MQTT_MESSAGE_DETAILS_LIST, *PMQTT_MESSAGE_DETAILS_LIST;

typedef struct DEVICE_METHOD_INFO_TAG
{
    STRING_HANDLE request_id;
} DEVICE_METHOD_INFO;

static void free_proxy_data(MQTTTRANSPORT_HANDLE_DATA* mqtt_transport_instance)
{
    if (mqtt_transport_instance->http_proxy_hostname != NULL)
    {
        free(mqtt_transport_instance->http_proxy_hostname);
        mqtt_transport_instance->http_proxy_hostname = NULL;
    }

    if (mqtt_transport_instance->http_proxy_username != NULL)
    {
        free(mqtt_transport_instance->http_proxy_username);
        mqtt_transport_instance->http_proxy_username = NULL;
    }

    if (mqtt_transport_instance->http_proxy_password != NULL)
    {
        free(mqtt_transport_instance->http_proxy_password);
        mqtt_transport_instance->http_proxy_password = NULL;
    }
}

static void set_saved_tls_options(PMQTTTRANSPORT_HANDLE_DATA transport, OPTIONHANDLER_HANDLE new_options)
{
    if (transport->saved_tls_options != NULL)
    {
        OptionHandler_Destroy(transport->saved_tls_options);
    }
    transport->saved_tls_options = new_options;
}

int IoTHubTransport_MQTT_Common_SetRetryPolicy(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_RETRY_POLICY retryPolicy, size_t retryTimeoutLimitInSeconds)
{
    int result;

    if (handle == NULL)
    {
        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_25_041: [**If any handle is NULL then IoTHubTransport_MQTT_Common_SetRetryPolicy shall return resultant line.] */
        LogError("Invalid handle parameter. NULL.");
        result = __FAILURE__;
    }
    else
    {
        RETRY_CONTROL_HANDLE new_retry_control_handle;

        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_006: [ IoTHubTransport_MQTT_Common_SetRetryPolicy shall set the retry logic by calling retry_control_create() with retry policy and retryTimeout as parameters]
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_009: [ If retry_control_create() fails then IoTHubTransport_MQTT_Common_SetRetryPolicy shall revert to previous retry policy and return non-zero value ]
        if ((new_retry_control_handle = retry_control_create(retryPolicy, (unsigned int)retryTimeoutLimitInSeconds)) == NULL)
        {
            LogError("Failed creating new retry control handle");
            result = __FAILURE__;
        }
        else
        {
            PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
            RETRY_CONTROL_HANDLE previous_retry_control_handle = transport_data->retry_control_handle;

            transport_data->retry_control_handle = new_retry_control_handle;
            retry_control_destroy(previous_retry_control_handle);

            /*Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_25_045: [**If retry logic for specified parameters of retry policy and retryTimeoutLimitInSeconds is created successfully then IoTHubTransport_MQTT_Common_SetRetryPolicy shall return 0]*/
            result = 0;
        }
    }

    return result;
}

static uint16_t get_next_packet_id(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->packetId+1 >= USHRT_MAX)
    {
        transport_data->packetId = 1;
    }
    else
    {
        transport_data->packetId++;
    }
    return transport_data->packetId;
}

static const char* retrieve_mqtt_return_codes(CONNECT_RETURN_CODE rtn_code)
{
    switch (rtn_code)
    {
        case CONNECTION_ACCEPTED:
            return "Accepted";
        case CONN_REFUSED_UNACCEPTABLE_VERSION:
            return "Unacceptable Version";
        case CONN_REFUSED_ID_REJECTED:
            return "Id Rejected";
        case CONN_REFUSED_SERVER_UNAVAIL:
            return "Server Unavailable";
        case CONN_REFUSED_BAD_USERNAME_PASSWORD:
            return "Bad Username/Password";
        case CONN_REFUSED_NOT_AUTHORIZED:
            return "Not Authorized";
        case CONN_REFUSED_UNKNOWN:
        default:
            return "Unknown";
    }
}

static int retrieve_device_method_rid_info(const char* resp_topic, STRING_HANDLE method_name, STRING_HANDLE request_id)
{
    int result;
    STRING_TOKENIZER_HANDLE token_handle = STRING_TOKENIZER_create_from_char(resp_topic);
    if (token_handle == NULL)
    {
        LogError("Failed creating token from device twin topic.");
        result = __FAILURE__;
    }
    else
    {
        STRING_HANDLE token_value;
        if ((token_value = STRING_new()) == NULL)
        {
            LogError("Failed allocating new string .");
            result = __FAILURE__;
        }
        else
        {
            size_t token_index = 0;
            size_t request_id_length = strlen(REQUEST_ID_PROPERTY);
            result = __FAILURE__;
            while (STRING_TOKENIZER_get_next_token(token_handle, token_value, "/") == 0)
            {
                if (token_index == 3)
                {
                    if (STRING_concat_with_STRING(method_name, token_value) != 0)
                    {
                        LogError("Failed STRING_concat_with_STRING.");
                        result = __FAILURE__;
                        break;
                    }
                }
                else if (token_index == 4)
                {
                    if (STRING_length(token_value) >= request_id_length)
                    {
                        const char* request_id_value = STRING_c_str(token_value);
                        if (memcmp(request_id_value, REQUEST_ID_PROPERTY, request_id_length) == 0)
                        {
                            if (STRING_concat(request_id, request_id_value+request_id_length) != 0)
                            {
                                LogError("Failed STRING_concat failed.");
                                result = __FAILURE__;
                            }
                            else
                            {
                                result = 0;
                            }
                            break;
                        }
                    }
                }
                token_index++;
            }
            STRING_delete(token_value);
        }
        STRING_TOKENIZER_destroy(token_handle);
    }
    return result;
}

static int parse_device_twin_topic_info(const char* resp_topic, bool* patch_msg, size_t* request_id, int* status_code)
{
    int result;
    STRING_TOKENIZER_HANDLE token_handle = STRING_TOKENIZER_create_from_char(resp_topic);
    if (token_handle == NULL)
    {
        LogError("Failed creating token from device twin topic.");
        result = __FAILURE__;
        *status_code = 0;
        *request_id = 0;
        *patch_msg = false;
    }
    else
    {
        STRING_HANDLE token_value;
        if ((token_value = STRING_new()) == NULL)
        {
            LogError("Failed allocating new string .");
            result = __FAILURE__;
            *status_code = 0;
            *request_id = 0;
            *patch_msg = false;
        }
        else
        {
            result = __FAILURE__;
            size_t token_count = 0;
            while (STRING_TOKENIZER_get_next_token(token_handle, token_value, "/") == 0)
            {
                if (token_count == 2)
                {
                    if (strcmp(STRING_c_str(token_value), "PATCH") == 0)
                    {
                        *patch_msg = true;
                        *status_code = 0;
                        *request_id = 0;
                        result = 0;
                        break;
                    }
                    else
                    {
                        *patch_msg = false;
                    }
                }
                else if (token_count == 3)
                {
                    *status_code = atol(STRING_c_str(token_value));
                    if (STRING_TOKENIZER_get_next_token(token_handle, token_value, "/?$rid=") == 0)
                    {
                        *request_id = (size_t)atol(STRING_c_str(token_value));
                    }
                    *patch_msg = false;
                    result = 0;
                    break;
                }
                token_count++;
            }
            STRING_delete(token_value);
        }
        STRING_TOKENIZER_destroy(token_handle);
    }
    return result;
}

static IOTHUB_IDENTITY_TYPE retrieve_topic_type(const char* topic_resp)
{
    IOTHUB_IDENTITY_TYPE type;
    size_t dev_twin_topic_len = sizeof(TOPIC_DEVICE_TWIN_PREFIX)-1;
    size_t dev_method_topic_len = sizeof(TOPIC_DEVICE_METHOD_PREFIX)-1;
    size_t topic_len = strlen(topic_resp);
    size_t len_to_use = (dev_twin_topic_len > dev_method_topic_len) ? dev_twin_topic_len : dev_method_topic_len;

    bool search_device_twin = true;
    bool search_device_method = true;
    if (topic_len < len_to_use)
    {
        search_device_twin = false;
        search_device_method = false;
    }
    else
    {
        size_t index = 0;
        for (index = 0; index < len_to_use && (search_device_twin || search_device_method); index++)
        {
            if (search_device_twin)
            {
                if (index > dev_twin_topic_len)
                {
                    search_device_twin = false;
                }
                else
                {
                    if (index == dev_twin_topic_len)
                    {
                        break;
                    }
                    else if (TOUPPER(TOPIC_DEVICE_TWIN_PREFIX[index]) != TOUPPER(topic_resp[index]))
                    {
                        search_device_twin = false;
                    }
                }
            }
            if (search_device_method)
            {
                if (index > dev_method_topic_len)
                {
                    search_device_method = false;
                }
                else
                {
                    if (index == dev_method_topic_len)
                    {
                        break;
                    }
                    else if (TOUPPER(TOPIC_DEVICE_METHOD_PREFIX[index]) != TOUPPER(topic_resp[index]))
                    {
                        search_device_method = false;
                    }
                }
            }
        }
    }
    if (search_device_twin)
    {
        type = IOTHUB_TYPE_DEVICE_TWIN;
    }
    else if (search_device_method)
    {
        type = IOTHUB_TYPE_DEVICE_METHODS;
    }
    else
    {
        type = IOTHUB_TYPE_TELEMETRY;
    }
    return type;
}

static void sendMsgComplete(IOTHUB_MESSAGE_LIST* iothubMsgList, PMQTTTRANSPORT_HANDLE_DATA transport_data, IOTHUB_CLIENT_CONFIRMATION_RESULT confirmResult)
{
    DLIST_ENTRY messageCompleted;
    DList_InitializeListHead(&messageCompleted);
    DList_InsertTailList(&messageCompleted, &(iothubMsgList->entry));
    IoTHubClient_LL_SendComplete(transport_data->llClientHandle, &messageCompleted, confirmResult);
}

static STRING_HANDLE addPropertiesTouMqttMessage(IOTHUB_MESSAGE_HANDLE iothub_message_handle, const char* eventTopic)
{
    STRING_HANDLE result = STRING_construct(eventTopic);
    const char* const* propertyKeys;
    const char* const* propertyValues;
    size_t propertyCount;
    size_t index = 0;

    // Construct Properties
    MAP_HANDLE properties_map = IoTHubMessage_Properties(iothub_message_handle);
    if (properties_map != NULL)
    {
        if (Map_GetInternals(properties_map, &propertyKeys, &propertyValues, &propertyCount) != MAP_OK)
        {
            LogError("Failed to get the internals of the property map.");
            STRING_delete(result);
            result = NULL;
        }
        else
        {
            if (propertyCount != 0)
            {
                for (index = 0; index < propertyCount && result != NULL; index++)
                {
                    if (STRING_sprintf(result, "%s=%s%s", propertyKeys[index], propertyValues[index], propertyCount - 1 == index ? "" : PROPERTY_SEPARATOR) != 0)
                    {
                        LogError("Failed construting property string.");
                        STRING_delete(result);
                        result = NULL;
                    }
                }
            }
        }
    }

    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_052: [ IoTHubTransport_MQTT_Common_DoWork shall check for the CorrelationId property and if found add the value as a system property in the format of $.cid=<id> ] */
    if (result != NULL)
    {
        const char* correlation_id = IoTHubMessage_GetCorrelationId(iothub_message_handle);
        if (correlation_id != NULL)
        {
            if (STRING_sprintf(result, "%s%%24.cid=%s", index == 0 ? "" : PROPERTY_SEPARATOR, correlation_id) != 0)
            {
                LogError("Failed setting correlation id.");
                STRING_delete(result);
                result = NULL;
            }
            index++;
        }
    }

    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_053: [ IoTHubTransport_MQTT_Common_DoWork shall check for the MessageId property and if found add the value as a system property in the format of $.mid=<id> ] */
    if (result != NULL)
    {
        const char* msg_id = IoTHubMessage_GetMessageId(iothub_message_handle);
        if (msg_id != NULL)
        {
            if (STRING_sprintf(result, "%s%%24.mid=%s", index == 0 ? "" : PROPERTY_SEPARATOR, msg_id) != 0)
            {
                LogError("Failed setting message id.");
                STRING_delete(result);
                result = NULL;
            }
            index++;
        }
    }

    // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_010: [ `IoTHubTransport_MQTT_Common_DoWork` shall check for the ContentType property and if found add the `value` as a system property in the format of `$.ct=<value>` ]
    if (result != NULL)
    {
        const char* content_type = IoTHubMessage_GetContentTypeSystemProperty(iothub_message_handle);
        if (content_type != NULL)
        {
            if (STRING_sprintf(result, "%s%%24.%s=%s", index == 0 ? "" : PROPERTY_SEPARATOR, CONTENT_TYPE_PROPERTY, content_type) != 0)
            {
                LogError("Failed setting content type.");
                STRING_delete(result);
                result = NULL;
            }
            index++;
        }
    }

    // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_011: [ `IoTHubTransport_MQTT_Common_DoWork` shall check for the ContentEncoding property and if found add the `value` as a system property in the format of `$.ce=<value>` ]
    if (result != NULL)
    {
        const char* content_encoding = IoTHubMessage_GetContentEncodingSystemProperty(iothub_message_handle);
        if (content_encoding != NULL)
        {
            if (STRING_sprintf(result, "%s%%24.%s=%s", index == 0 ? "" : PROPERTY_SEPARATOR, CONTENT_ENCODING_PROPERTY, content_encoding) != 0)
            {
                LogError("Failed setting content encoding.");
                STRING_delete(result);
                result = NULL;
            }
            index++;
        }
    }

    return result;
}

static int publish_mqtt_telemetry_msg(PMQTTTRANSPORT_HANDLE_DATA transport_data, MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry, const unsigned char* payload, size_t len)
{
    int result;
    STRING_HANDLE msgTopic = addPropertiesTouMqttMessage(mqttMsgEntry->iotHubMessageEntry->messageHandle, STRING_c_str(transport_data->topic_MqttEvent));
    if (msgTopic == NULL)
    {
        LogError("Failed adding properties to mqtt message");
        result = __FAILURE__;
    }
    else
    {
        MQTT_MESSAGE_HANDLE mqttMsg = mqttmessage_create(mqttMsgEntry->packet_id, STRING_c_str(msgTopic), DELIVER_AT_LEAST_ONCE, payload, len);
        if (mqttMsg == NULL)
        {
            LogError("Failed creating mqtt message");
            result = __FAILURE__;
        }
        else
        {
            if (tickcounter_get_current_ms(transport_data->msgTickCounter, &mqttMsgEntry->msgPublishTime) != 0)
            {
                LogError("Failed retrieving tickcounter info");
                result = __FAILURE__;
            }
            else
            {
                if (mqtt_client_publish(transport_data->mqttClient, mqttMsg) != 0)
                {
                    LogError("Failed attempting to publish mqtt message");
                    result = __FAILURE__;
                }
                else
                {
                    mqttMsgEntry->retryCount++;
                    result = 0;
                }
            }
            mqttmessage_destroy(mqttMsg);
        }
        STRING_delete(msgTopic);
    }
    return result;
}

static int publish_device_method_message(MQTTTRANSPORT_HANDLE_DATA* transport_data, int status_code, STRING_HANDLE request_id, const unsigned char* response, size_t response_size)
{
    int result;
    uint16_t packet_id = get_next_packet_id(transport_data);

    STRING_HANDLE msg_topic = STRING_construct_sprintf(DEVICE_METHOD_RESPONSE_TOPIC, status_code, STRING_c_str(request_id) );
    if (msg_topic == NULL)
    {
        LogError("Failed constructing message topic.");
        result = __FAILURE__;
    }
    else
    {
        MQTT_MESSAGE_HANDLE mqtt_get_msg = mqttmessage_create(packet_id, STRING_c_str(msg_topic), DELIVER_AT_MOST_ONCE, response, response_size);
        if (mqtt_get_msg == NULL)
        {
            LogError("Failed constructing mqtt message.");
            result = __FAILURE__;
        }
        else
        {
            if (mqtt_client_publish(transport_data->mqttClient, mqtt_get_msg) != 0)
            {
                LogError("Failed publishing to mqtt client.");
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
            mqttmessage_destroy(mqtt_get_msg);
        }
        STRING_delete(msg_topic);
    }
    return result;
}

static int publish_device_twin_get_message(MQTTTRANSPORT_HANDLE_DATA* transport_data)
{
    int result;
    MQTT_DEVICE_TWIN_ITEM* mqtt_info = (MQTT_DEVICE_TWIN_ITEM*)malloc(sizeof(MQTT_DEVICE_TWIN_ITEM));
    if (mqtt_info == NULL)
    {
        LogError("Failed allocating device twin data.");
        result = __FAILURE__;
    }
    else
    {
        mqtt_info->packet_id = get_next_packet_id(transport_data);
        mqtt_info->iothub_msg_id = 0;
        mqtt_info->device_twin_msg_type = RETRIEVE_PROPERTIES;
        mqtt_info->retryCount = 0;
        mqtt_info->msgPublishTime = 0;
        mqtt_info->iothub_type = IOTHUB_TYPE_DEVICE_TWIN;
        mqtt_info->device_twin_data = NULL;
        STRING_HANDLE msg_topic = STRING_construct_sprintf(GET_PROPERTIES_TOPIC, mqtt_info->packet_id);
        if (msg_topic == NULL)
        {
            LogError("Failed constructing get Prop topic.");
            free(mqtt_info);
            result = __FAILURE__;
        }
        else
        {
            MQTT_MESSAGE_HANDLE mqtt_get_msg = mqttmessage_create(mqtt_info->packet_id, STRING_c_str(msg_topic), DELIVER_AT_MOST_ONCE, NULL, 0);
            if (mqtt_get_msg == NULL)
            {
                LogError("Failed constructing mqtt message.");
                free(mqtt_info);
                result = __FAILURE__;
            }
            else
            {
                if (mqtt_client_publish(transport_data->mqttClient, mqtt_get_msg) != 0)
                {
                    LogError("Failed publishing to mqtt client.");
                    free(mqtt_info);
                    result = __FAILURE__;
                }
                else
                {
                    DList_InsertTailList(&transport_data->ack_waiting_queue, &mqtt_info->entry);
                    result = 0;
                }
                mqttmessage_destroy(mqtt_get_msg);
            }
            STRING_delete(msg_topic);
        }
    }
    return result;
}

static int publish_device_twin_message(MQTTTRANSPORT_HANDLE_DATA* transport_data, IOTHUB_DEVICE_TWIN* device_twin_info, MQTT_DEVICE_TWIN_ITEM* mqtt_info)
{
    int result;
    mqtt_info->packet_id = get_next_packet_id(transport_data);
    mqtt_info->device_twin_msg_type = REPORTED_STATE;
    STRING_HANDLE msgTopic = STRING_construct_sprintf(REPORTED_PROPERTIES_TOPIC, mqtt_info->packet_id);
    if (msgTopic == NULL)
    {
        LogError("Failed constructing reported prop topic.");
        result = __FAILURE__;
    }
    else
    {
        const CONSTBUFFER* data_buff = CONSTBUFFER_GetContent(device_twin_info->report_data_handle);
        MQTT_MESSAGE_HANDLE mqtt_rpt_msg = mqttmessage_create(mqtt_info->packet_id, STRING_c_str(msgTopic), DELIVER_AT_MOST_ONCE, data_buff->buffer, data_buff->size);
        if (mqtt_rpt_msg == NULL)
        {
            LogError("Failed creating mqtt message");
            result = __FAILURE__;
        }
        else
        {
            if (tickcounter_get_current_ms(transport_data->msgTickCounter, &mqtt_info->msgPublishTime) != 0)
            {
                LogError("Failed retrieving tickcounter info");
                result = __FAILURE__;
            }
            else
            {
                if (mqtt_client_publish(transport_data->mqttClient, mqtt_rpt_msg) != 0)
                {
                    LogError("Failed publishing mqtt message");
                    result = __FAILURE__;
                }
                else
                {
                    mqtt_info->retryCount++;
                    result = 0;
                }
            }
            mqttmessage_destroy(mqtt_rpt_msg);
        }
        STRING_delete(msgTopic);
    }
    return result;
}

static bool isSystemProperty(const char* tokenData)
{
    bool result = false;
    size_t propCount = sizeof(sysPropList)/sizeof(sysPropList[0]);
    size_t index = 0;
    for (index = 0; index < propCount; index++)
    {
        if (memcmp(tokenData, sysPropList[index].propName, sysPropList[index].propLength) == 0)
        {
            result = true;
            break;
        }
    }
    return result;
}

static int extractMqttProperties(IOTHUB_MESSAGE_HANDLE IoTHubMessage, const char* topic_name)
{
    int result;
    STRING_HANDLE mqttTopic = STRING_construct(topic_name);
    if (mqttTopic == NULL)
    {
        LogError("Failure constructing string topic name.");
        result = __FAILURE__;
    }
    else
    {
        STRING_TOKENIZER_HANDLE token = STRING_TOKENIZER_create(mqttTopic);
        if (token != NULL)
        {
            MAP_HANDLE propertyMap = IoTHubMessage_Properties(IoTHubMessage);
            if (propertyMap == NULL)
            {
                LogError("Failure to retrieve IoTHubMessage_properties.");
                result = __FAILURE__;
            }
            else
            {
                STRING_HANDLE output = STRING_new();
                if (output == NULL)
                {
                    LogError("Failure to allocate STRING_new.");
                    result = __FAILURE__;
                }
                else
                {
                    result = 0;

                    while (STRING_TOKENIZER_get_next_token(token, output, PROPERTY_SEPARATOR) == 0 && result == 0)
                    {
                        const char* tokenData = STRING_c_str(output);
                        size_t tokenLen = strlen(tokenData);
                        if (tokenData == NULL || tokenLen == 0)
                        {
                            break;
                        }
                        else
                        {
                            if (isSystemProperty(tokenData))
                            {
                                const char* iterator = tokenData;
                                while (iterator != NULL && *iterator != '\0' && result == 0)
                                {
                                    if (*iterator == '=')
                                    {
                                        size_t nameLen = iterator - tokenData;
                                        char* propName = malloc(nameLen + 1);

                                        size_t valLen = tokenLen - (nameLen + 1) + 1;
                                        char* propValue = malloc(valLen + 1);

                                        if (propName == NULL || propValue == NULL)
                                        {
                                            LogError("Failed allocating property name (%p) and/or value (%p)", propName, propValue);
                                            result = __FAILURE__;
                                        }
                                        else
                                        {
                                            strncpy(propName, tokenData, nameLen);
                                            propName[nameLen] = '\0';

                                            strncpy(propValue, iterator + 1, valLen);
                                            propValue[valLen] = '\0';

                                            if (nameLen > 2)
                                            {
                                                if (nameLen > 3)
                                                {
                                                    if (strcmp((const char*)&propName[nameLen - 3], MESSAGE_ID_PROPERTY) == 0)
                                                    {
                                                        if (IoTHubMessage_SetMessageId(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
                                                        {
                                                            LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'messageId' property.");
                                                            result = __FAILURE__;
                                                        }
                                                    }
                                                    else if (strcmp((const char*)&propName[nameLen - 3], CORRELATION_ID_PROPERTY) == 0)
                                                    {
                                                        if (IoTHubMessage_SetCorrelationId(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
                                                        {
                                                            LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'correlationId' property.");
                                                            result = __FAILURE__;
                                                        }
                                                    }
                                                }

                                                // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_012: [ If type is IOTHUB_TYPE_TELEMETRY and the system property `$.ct` is defined, its value shall be set on the IOTHUB_MESSAGE_HANDLE's ContentType property ]
                                                if (strcmp((const char*)&propName[nameLen - 2], CONTENT_TYPE_PROPERTY) == 0)
                                                {
                                                    if (IoTHubMessage_SetContentTypeSystemProperty(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
                                                    {
                                                        LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'customContentType' property.");
                                                        result = __FAILURE__;
                                                    }
                                                }
                                                // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_013: [ If type is IOTHUB_TYPE_TELEMETRY and the system property `$.ce` is defined, its value shall be set on the IOTHUB_MESSAGE_HANDLE's ContentEncoding property ]
                                                else if (strcmp((const char*)&propName[nameLen - 2], CONTENT_ENCODING_PROPERTY) == 0)
                                                {
                                                    if (IoTHubMessage_SetContentEncodingSystemProperty(IoTHubMessage, propValue) != IOTHUB_MESSAGE_OK)
                                                    {
                                                        LogError("Failed to set IOTHUB_MESSAGE_HANDLE 'contentEncoding' property.");
                                                        result = __FAILURE__;
                                                    }
                                                }
                                            }
                                        }
                                        free(propName);
                                        free(propValue);

                                        break;
                                    }
                                    iterator++;
                                }
                            }
                            else
                            {
                                const char* iterator = tokenData;
                                while (iterator != NULL && *iterator != '\0' && result == 0)
                                {
                                    if (*iterator == '=')
                                    {
                                        size_t nameLen = iterator - tokenData;
                                        char* propName = malloc(nameLen + 1);

                                        size_t valLen = tokenLen - (nameLen + 1) + 1;
                                        char* propValue = malloc(valLen + 1);

                                        if (propName == NULL || propValue == NULL)
                                        {
                                            result = __FAILURE__;
                                        }
                                        else
                                        {
                                            strncpy(propName, tokenData, nameLen);
                                            propName[nameLen] = '\0';

                                            strncpy(propValue, iterator + 1, valLen);
                                            propValue[valLen] = '\0';

                                            if (Map_AddOrUpdate(propertyMap, propName, propValue) != MAP_OK)
                                            {
                                                LogError("Map_AddOrUpdate failed.");
                                                result = __FAILURE__;
                                            }
                                        }
                                        free(propName);
                                        free(propValue);

                                        break;
                                    }
                                    iterator++;
                                }
                            }
                        }
                    }
                    STRING_delete(output);
                }
            }
            STRING_TOKENIZER_destroy(token);
        }
        else
        {
            LogError("Unable to create Tokenizer object.");
            result = __FAILURE__;
        }
        STRING_delete(mqttTopic);
    }
    return result;
}

static void mqtt_notification_callback(MQTT_MESSAGE_HANDLE msgHandle, void* callbackCtx)
{
    /* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_051: [ If msgHandle or callbackCtx is NULL, mqtt_notification_callback shall do nothing. ] */
    if (msgHandle != NULL && callbackCtx != NULL)
    {
        /* Tests_SRS_IOTHUB_MQTT_TRANSPORT_07_052: [ mqtt_notification_callback shall extract the topic Name from the MQTT_MESSAGE_HANDLE. ] */
        const char* topic_resp = mqttmessage_getTopicName(msgHandle);
        if (topic_resp == NULL)
        {
            LogError("Failure: NULL topic name encountered");
        }
        else
        {
            PMQTTTRANSPORT_HANDLE_DATA transportData = (PMQTTTRANSPORT_HANDLE_DATA)callbackCtx;

            IOTHUB_IDENTITY_TYPE type = retrieve_topic_type(topic_resp);
            if (type == IOTHUB_TYPE_DEVICE_TWIN)
            {
                size_t request_id;
                int status_code;
                bool notification_msg;
                if (parse_device_twin_topic_info(topic_resp, &notification_msg, &request_id, &status_code) != 0)
                {
                    LogError("Failure: parsing device topic info");
                }
                else
                {
                    const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(msgHandle);
                    if (notification_msg)
                    {
                        IoTHubClient_LL_RetrievePropertyComplete(transportData->llClientHandle, DEVICE_TWIN_UPDATE_PARTIAL, payload->message, payload->length);
                    }
                    else
                    {
                        PDLIST_ENTRY dev_twin_item = transportData->ack_waiting_queue.Flink;
                        while (dev_twin_item != &transportData->ack_waiting_queue)
                        {
                            DLIST_ENTRY saveListEntry;
                            saveListEntry.Flink = dev_twin_item->Flink;
                            MQTT_DEVICE_TWIN_ITEM* msg_entry = containingRecord(dev_twin_item, MQTT_DEVICE_TWIN_ITEM, entry);
                            if (request_id == msg_entry->packet_id)
                            {
                                (void)DList_RemoveEntryList(dev_twin_item);
                                if (msg_entry->device_twin_msg_type == RETRIEVE_PROPERTIES)
                                {
                                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ If type is IOTHUB_TYPE_DEVICE_TWIN, then on success if msg_type is RETRIEVE_PROPERTIES then mqtt_notification_callback shall call IoTHubClient_LL_RetrievePropertyComplete... ] */
                                    IoTHubClient_LL_RetrievePropertyComplete(transportData->llClientHandle, DEVICE_TWIN_UPDATE_COMPLETE, payload->message, payload->length);
                                }
                                else
                                {
                                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_055: [ if device_twin_msg_type is not RETRIEVE_PROPERTIES then mqtt_notification_callback shall call IoTHubClient_LL_ReportedStateComplete ] */
                                    IoTHubClient_LL_ReportedStateComplete(transportData->llClientHandle, msg_entry->iothub_msg_id, status_code);
                                }
                                free(msg_entry);
                                break;
                            }
                            dev_twin_item = saveListEntry.Flink;
                        }
                    }
                }
            }
            else if (type == IOTHUB_TYPE_DEVICE_METHODS)
            {
                STRING_HANDLE method_name = STRING_new();
                if (method_name == NULL)
                {
                    LogError("Failure: allocating method_name string value");
                }
                else
                {
                    DEVICE_METHOD_INFO* dev_method_info = malloc(sizeof(DEVICE_METHOD_INFO) );
                    if (dev_method_info == NULL)
                    {
                        LogError("Failure: allocating DEVICE_METHOD_INFO object");
                    }
                    else
                    {
                        dev_method_info->request_id = STRING_new();
                        if (dev_method_info->request_id == NULL)
                        {
                            LogError("Failure constructing request_id string");
                            free(dev_method_info);
                        }
                        else if (retrieve_device_method_rid_info(topic_resp, method_name, dev_method_info->request_id) != 0)
                        {
                            LogError("Failure: retrieve device topic info");
                            STRING_delete(dev_method_info->request_id);
                            free(dev_method_info);
                        }
                        else
                        {
                            /* CodesSRS_IOTHUB_MQTT_TRANSPORT_07_053: [ If type is IOTHUB_TYPE_DEVICE_METHODS, then on success mqtt_notification_callback shall call IoTHubClient_LL_DeviceMethodComplete. ] */
                            const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(msgHandle);
                            if (IoTHubClient_LL_DeviceMethodComplete(transportData->llClientHandle, STRING_c_str(method_name), payload->message, payload->length, (void*)dev_method_info) != 0)
                            {
                                LogError("Failure: IoTHubClient_LL_DeviceMethodComplete");
                                STRING_delete(dev_method_info->request_id);
                                free(dev_method_info);
                            }
                        }
                    }
                    STRING_delete(method_name);
                }
            }
            else
            {
                const APP_PAYLOAD* appPayload = mqttmessage_getApplicationMsg(msgHandle);
                IOTHUB_MESSAGE_HANDLE IoTHubMessage = IoTHubMessage_CreateFromByteArray(appPayload->message, appPayload->length);
                if (IoTHubMessage == NULL)
                {
                    LogError("Failure: IotHub Message creation has failed.");
                }
                else
                {
                    // Will need to update this when the service has messages that can be rejected
                    if (extractMqttProperties(IoTHubMessage, topic_resp) != 0)
                    {
                        LogError("failure extracting mqtt properties.");
                    }
                    else
                    {
                        MESSAGE_CALLBACK_INFO* messageData = (MESSAGE_CALLBACK_INFO*)malloc(sizeof(MESSAGE_CALLBACK_INFO));
                        if (messageData == NULL)
                        {
                            LogError("malloc failed");
                        }
                        else
                        {
                            messageData->messageHandle = IoTHubMessage;
                            messageData->transportContext = NULL;

                            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_056: [ If type is IOTHUB_TYPE_TELEMETRY, then on success mqtt_notification_callback shall call IoTHubClient_LL_MessageCallback. ] */
                            if (!IoTHubClient_LL_MessageCallback(transportData->llClientHandle, messageData))
                            {
                                LogError("IoTHubClient_LL_MessageCallback returned false");

                                IoTHubMessage_Destroy(IoTHubMessage);
                                free(messageData);
                            }
                        }
                    }
                }
            }
        }
    }
}

static void mqtt_operation_complete_callback(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_RESULT actionResult, const void* msgInfo, void* callbackCtx)
{
    (void)handle;
    if (callbackCtx != NULL)
    {
        PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)callbackCtx;

        switch (actionResult)
        {
            case MQTT_CLIENT_ON_PUBLISH_ACK:
            case MQTT_CLIENT_ON_PUBLISH_COMP:
            {
                const PUBLISH_ACK* puback = (const PUBLISH_ACK*)msgInfo;
                if (puback != NULL)
                {
                    PDLIST_ENTRY currentListEntry = transport_data->telemetry_waitingForAck.Flink;
                    while (currentListEntry != &transport_data->telemetry_waitingForAck)
                    {
                        MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry = containingRecord(currentListEntry, MQTT_MESSAGE_DETAILS_LIST, entry);
                        DLIST_ENTRY saveListEntry;
                        saveListEntry.Flink = currentListEntry->Flink;

                        if (puback->packetId == mqttMsgEntry->packet_id)
                        {
                            (void)DList_RemoveEntryList(currentListEntry); //First remove the item from Waiting for Ack List.
                            sendMsgComplete(mqttMsgEntry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_OK);
                            free(mqttMsgEntry);
                        }
                        currentListEntry = saveListEntry.Flink;
                    }
                }
                else
                {
                    LogError("Failure: MQTT_CLIENT_ON_PUBLISH_ACK publish_ack structure NULL.");
                }
                break;
            }
            case MQTT_CLIENT_ON_CONNACK:
            {
                const CONNECT_ACK* connack = (const CONNECT_ACK*)msgInfo;
                if (connack != NULL)
                {
                    if (connack->returnCode == CONNECTION_ACCEPTED)
                    {
                        // The connect packet has been acked
                        transport_data->currPacketState = CONNACK_TYPE;
                        transport_data->isRecoverableError = true;
                        transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_CONNECTED;

                        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_008: [ Upon successful connection the retry control shall be reset using retry_control_reset() ]
                        retry_control_reset(transport_data->retry_control_handle);

                        IoTHubClient_LL_ConnectionStatusCallBack(transport_data->llClientHandle, IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK);
                    }
                    else
                    {
                        if (connack->returnCode == CONN_REFUSED_BAD_USERNAME_PASSWORD)
                        {
                            transport_data->isRecoverableError = false;
                            IoTHubClient_LL_ConnectionStatusCallBack(transport_data->llClientHandle, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL);
                        }
                        else if (connack->returnCode == CONN_REFUSED_NOT_AUTHORIZED)
                        {
                            IoTHubClient_LL_ConnectionStatusCallBack(transport_data->llClientHandle, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED);
                        }
                        else if (connack->returnCode == CONN_REFUSED_UNACCEPTABLE_VERSION)
                        {
                            transport_data->isRecoverableError = false;
                        }
                        LogError("Connection Not Accepted: 0x%x: %s", connack->returnCode, retrieve_mqtt_return_codes(connack->returnCode) );
                        (void)mqtt_client_disconnect(transport_data->mqttClient, NULL, NULL);
                        transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
                        transport_data->currPacketState = PACKET_TYPE_ERROR;
                    }
                }
                else
                {
                    LogError("MQTT_CLIENT_ON_CONNACK CONNACK parameter is NULL.");
                }
                break;
            }
            case MQTT_CLIENT_ON_SUBSCRIBE_ACK:
            {
                const SUBSCRIBE_ACK* suback = (const SUBSCRIBE_ACK*)msgInfo;
                if (suback != NULL)
                {
                    size_t index = 0;
                    for (index = 0; index < suback->qosCount; index++)
                    {
                        if (suback->qosReturn[index] == DELIVER_FAILURE)
                        {
                            LogError("Subscribe delivery failure of subscribe %lu", index);
                        }
                    }
                    // The connect packet has been acked
                    transport_data->currPacketState = SUBACK_TYPE;
                }
                else
                {
                    LogError("Failure: MQTT_CLIENT_ON_SUBSCRIBE_ACK SUBSCRIBE_ACK parameter is NULL.");
                }
                break;
            }
            case MQTT_CLIENT_ON_PUBLISH_RECV:
            case MQTT_CLIENT_ON_PUBLISH_REL:
            {
                // Currently not used
                break;
            }
            case MQTT_CLIENT_ON_DISCONNECT:
            {
                // Close the client so we can reconnect again
                transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
                transport_data->currPacketState = DISCONNECT_TYPE;
                break;
            }
            case MQTT_CLIENT_ON_UNSUBSCRIBE_ACK:
            case MQTT_CLIENT_ON_PING_RESPONSE:
            default:
            {
                break;
            } 
        }
    }
}

static void DisconnectFromClient(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->xioTransport != NULL)
    {
        OPTIONHANDLER_HANDLE options = xio_retrieveoptions(transport_data->xioTransport);
        set_saved_tls_options(transport_data, options);

        (void)mqtt_client_disconnect(transport_data->mqttClient, NULL, NULL);
        xio_destroy(transport_data->xioTransport);
        transport_data->xioTransport = NULL;

        transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
        transport_data->currPacketState = DISCONNECT_TYPE;
    }
}

static void UpdateSubscribeFlag(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->topic_MqttMessage != NULL)
    {
        transport_data->topics_ToSubscribe |= SUBSCRIBE_TELEMETRY_TOPIC;
    }
    if (transport_data->topic_GetState != NULL)
    {
        transport_data->topics_ToSubscribe |= SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
    }
    if (transport_data->topic_NotifyState != NULL)
    {
        transport_data->topics_ToSubscribe |= SUBSCRIBE_NOTIFICATION_STATE_TOPIC;
    }
    if (transport_data->topic_DeviceMethods != NULL)
    {
        transport_data->topics_ToSubscribe |= SUBSCRIBE_DEVICE_METHOD_TOPIC;
    }
}

static void mqtt_error_callback(MQTT_CLIENT_HANDLE handle, MQTT_CLIENT_EVENT_ERROR error, void* callbackCtx)
{
    if (callbackCtx != NULL)
    {
        PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)callbackCtx;
        switch (error)
        {
            case MQTT_CLIENT_CONNECTION_ERROR:
            {
                IoTHubClient_LL_ConnectionStatusCallBack(transport_data->llClientHandle, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_NETWORK);
                break;
            }
            case MQTT_CLIENT_COMMUNICATION_ERROR:
            {
                IoTHubClient_LL_ConnectionStatusCallBack(transport_data->llClientHandle, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR);
                break;
            }
            case MQTT_CLIENT_NO_PING_RESPONSE:
            {
                LogError("Mqtt Ping Response was not encountered.  Reconnecting device...");
                DisconnectFromClient(transport_data);
                break;
            }
            case MQTT_CLIENT_PARSE_ERROR:
            case MQTT_CLIENT_MEMORY_ERROR:
            case MQTT_CLIENT_UNKNOWN_ERROR: 
            {
                LogError("INTERNAL ERROR: unexpected error value received %s", ENUM_TO_STRING(MQTT_CLIENT_EVENT_ERROR, error));
                break;
            }
        }
        transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
        transport_data->currPacketState = PACKET_TYPE_ERROR;
        transport_data->device_twin_get_sent = false;
        UpdateSubscribeFlag(transport_data);
    }
    else
    {
        LogError("Failure: mqtt called back with null context.");
    }
}

static void SubscribeToMqttProtocol(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    if (transport_data->topics_ToSubscribe != UNSUBSCRIBE_FROM_TOPIC)
    {
        uint32_t topic_subscription = 0;
        size_t subscribe_count = 0;
        SUBSCRIBE_PAYLOAD subscribe[SUBSCRIBE_TOPIC_COUNT];
        if ((transport_data->topic_MqttMessage != NULL) && (SUBSCRIBE_TELEMETRY_TOPIC & transport_data->topics_ToSubscribe))
        {
            subscribe[subscribe_count].subscribeTopic = STRING_c_str(transport_data->topic_MqttMessage);
            subscribe[subscribe_count].qosReturn = DELIVER_AT_LEAST_ONCE;
            topic_subscription |= SUBSCRIBE_TELEMETRY_TOPIC;
            subscribe_count++;
        }
        if ((transport_data->topic_GetState != NULL) && (SUBSCRIBE_GET_REPORTED_STATE_TOPIC & transport_data->topics_ToSubscribe))
        {
            subscribe[subscribe_count].subscribeTopic = STRING_c_str(transport_data->topic_GetState);
            subscribe[subscribe_count].qosReturn = DELIVER_AT_MOST_ONCE;
            topic_subscription |= SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
            subscribe_count++;
        }
        if ((transport_data->topic_NotifyState != NULL) && (SUBSCRIBE_NOTIFICATION_STATE_TOPIC & transport_data->topics_ToSubscribe))
        {
            subscribe[subscribe_count].subscribeTopic = STRING_c_str(transport_data->topic_NotifyState);
            subscribe[subscribe_count].qosReturn = DELIVER_AT_MOST_ONCE;
            topic_subscription |= SUBSCRIBE_NOTIFICATION_STATE_TOPIC;
            subscribe_count++;
        }
        if ((transport_data->topic_DeviceMethods != NULL) && (SUBSCRIBE_DEVICE_METHOD_TOPIC & transport_data->topics_ToSubscribe))
        {
            subscribe[subscribe_count].subscribeTopic = STRING_c_str(transport_data->topic_DeviceMethods);
            subscribe[subscribe_count].qosReturn = DELIVER_AT_MOST_ONCE;
            topic_subscription |= SUBSCRIBE_DEVICE_METHOD_TOPIC;
            subscribe_count++;
        }

        if (subscribe_count != 0)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_016: [IoTHubTransport_MQTT_Common_Subscribe shall call mqtt_client_subscribe to subscribe to the Message Topic.] */
            if (mqtt_client_subscribe(transport_data->mqttClient, get_next_packet_id(transport_data), subscribe, subscribe_count) != 0)
            {
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_017: [Upon failure IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
                LogError("Failure: mqtt_client_subscribe returned error.");
            }
            else
            {
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_018: [On success IoTHubTransport_MQTT_Common_Subscribe shall return 0.] */
                transport_data->topics_ToSubscribe &= ~topic_subscription;
                transport_data->currPacketState = SUBSCRIBE_TYPE;
            }
        }
        else
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_017: [Upon failure IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
            LogError("Failure: subscribe_topic is empty.");
        }
        transport_data->currPacketState = SUBSCRIBE_TYPE;
    }
    else
    {
        transport_data->currPacketState = PUBLISH_TYPE;
    }
}

static const unsigned char* RetrieveMessagePayload(IOTHUB_MESSAGE_HANDLE messageHandle, size_t* length)
{
    const unsigned char* result;

    IOTHUBMESSAGE_CONTENT_TYPE contentType = IoTHubMessage_GetContentType(messageHandle);
    if (contentType == IOTHUBMESSAGE_BYTEARRAY)
    {
        if (IoTHubMessage_GetByteArray(messageHandle, &result, length) != IOTHUB_MESSAGE_OK)
        {
            LogError("Failure result from IoTHubMessage_GetByteArray");
            result = NULL;
            *length = 0;
        }
    }
    else if (contentType == IOTHUBMESSAGE_STRING)
    {
        result = (const unsigned char*)IoTHubMessage_GetString(messageHandle);
        if (result == NULL)
        {
            LogError("Failure result from IoTHubMessage_GetString");
            result = NULL;
            *length = 0;
        }
        else
        {
            *length = strlen((const char*)result);
        }
    }
    else
    {
        result = NULL;
        *length = 0;
    }
    return result;
}

static int GetTransportProviderIfNecessary(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result;

    if (transport_data->xioTransport == NULL)
    {
        // construct address
        const char* hostAddress = STRING_c_str(transport_data->hostAddress);
        MQTT_TRANSPORT_PROXY_OPTIONS mqtt_proxy_options;

        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_011: [ If no `proxy_data` option has been set, NULL shall be passed as the argument `mqtt_transport_proxy_options` when calling the function `get_io_transport` passed in `IoTHubTransport_MQTT_Common__Create`. ]*/
        mqtt_proxy_options.host_address = transport_data->http_proxy_hostname;
        mqtt_proxy_options.port = transport_data->http_proxy_port;
        mqtt_proxy_options.username = transport_data->http_proxy_username;
        mqtt_proxy_options.password = transport_data->http_proxy_password;

        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_010: [ If the `proxy_data` option has been set, the proxy options shall be filled in the argument `mqtt_transport_proxy_options` when calling the function `get_io_transport` passed in `IoTHubTransport_MQTT_Common__Create` to obtain the underlying IO handle. ]*/
        transport_data->xioTransport = transport_data->get_io_transport(hostAddress, (transport_data->http_proxy_hostname == NULL) ? NULL : &mqtt_proxy_options);
        if (transport_data->xioTransport == NULL)
        {
            LogError("Unable to create the lower level TLS layer.");
            result = __FAILURE__;
        }
        else
        {
            if (IoTHubClient_Auth_Get_Credential_Type(transport_data->authorization_module) == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
            {
                if (IoTHubClient_Auth_Set_xio_Certificate(transport_data->authorization_module, transport_data->xioTransport) != 0)
                {
                    LogError("Unable to create the lower level TLS layer.");
                    result = __FAILURE__;
                }
                else
                {
                    result = 0;
                }
            }
            else
            {
                result = 0;
            }

            if (result == 0)
            {
                if (transport_data->saved_tls_options != NULL)
                {
                    if (OptionHandler_FeedOptions(transport_data->saved_tls_options, transport_data->xioTransport) != OPTIONHANDLER_OK)
                    {
                        LogError("Failed feeding existing options to new TLS instance.");
                        result = __FAILURE__;
                    }
                    else
                    {
                        // The tlsio has the options, so our copy can be deleted
                        set_saved_tls_options(transport_data, NULL);
                        result = 0;
                    }
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    else
    {
        result = 0;
    }
    return result;
}

static int is_key_validate(const IOTHUBTRANSPORT_CONFIG* config)
{
    int result;
    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(config->auth_module_handle);
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_X509 || cred_type == IOTHUB_CREDENTIAL_TYPE_X509_ECC)
    {
        result = 0;
    }
    else
    {
        if (config->upperConfig->deviceKey == NULL && config->upperConfig->deviceSasToken == NULL)
        {
            if (IoTHubClient_Auth_Get_DeviceKey(config->auth_module_handle) == NULL)
            {
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

static int SendMqttConnectMsg(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result;

    char* sasToken = NULL;
    result = 0;

    IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(transport_data->authorization_module);
    if (cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY || cred_type == IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH)
    {
        size_t secSinceEpoch = (size_t)(difftime(get_time(NULL), EPOCH_TIME_T_VALUE) + 0);
        size_t expiryTime = secSinceEpoch + SAS_TOKEN_DEFAULT_LIFETIME;
        sasToken = IoTHubClient_Auth_Get_SasToken(transport_data->authorization_module, STRING_c_str(transport_data->devicesPath), expiryTime);
        if (sasToken == NULL)
        {
            LogError("failure getting sas token from IoTHubClient_Auth_Get_SasToken.");
            result = __FAILURE__;
        }
    }
    else if (cred_type == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN)
    {
        SAS_TOKEN_STATUS token_status = IoTHubClient_Auth_Is_SasToken_Valid(transport_data->authorization_module);
        if (token_status == SAS_TOKEN_STATUS_INVALID)
        {
            IoTHubClient_LL_ConnectionStatusCallBack(transport_data->llClientHandle, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN);
            result = __FAILURE__;
        }
        else if (token_status == SAS_TOKEN_STATUS_FAILED)
        {
            IoTHubClient_LL_ConnectionStatusCallBack(transport_data->llClientHandle, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL);
            result = __FAILURE__;
        }
        else
        {
            sasToken = IoTHubClient_Auth_Get_SasToken(transport_data->authorization_module, NULL, 0);
            if (sasToken == NULL)
            {
                LogError("failure getting sas Token.");
                result = __FAILURE__;
            }
        }
    }

    if (result == 0)
    {
        if (!transport_data->isProductInfoSet)
        {
            // This requires the iothubClientHandle, which sadly the MQTT transport only gets on DoWork, so this code still needs to remain here.
            // The correct place for this would be in the Create method, but we don't get the client handle there.
            // Also, when device multiplexing is used, the customer creates the transport directly and explicitly, when the client is still not created.
            // This will be a major hurdle when we add device multiplexing to MQTT transport.

            void* product_info;
            STRING_HANDLE clone;
            if ((IoTHubClient_LL_GetOption(transport_data->llClientHandle, OPTION_PRODUCT_INFO, &product_info) == IOTHUB_CLIENT_ERROR) || (product_info == NULL))
            {
                clone = STRING_construct_sprintf("%s%%2F%s", CLIENT_DEVICE_TYPE_PREFIX, IOTHUB_SDK_VERSION);
            }
            else
            {
                clone = URL_Encode(product_info);
            }

            if (clone == NULL)
            {
                LogError("Failed obtaining the product info");
            }
            else
            {
                if (STRING_concat_with_STRING(transport_data->configPassedThroughUsername, clone) != 0)
                {
                    LogError("Failed concatenating the product info");
                }
                else
                {
                    transport_data->isProductInfoSet = true;
                }

                STRING_delete(clone);
            }
        }

        MQTT_CLIENT_OPTIONS options = { 0 };
        options.clientId = (char*)STRING_c_str(transport_data->device_id);
        options.willMessage = NULL;
        options.username = (char*)STRING_c_str(transport_data->configPassedThroughUsername);
        if (sasToken != NULL)
        {
            options.password = sasToken;
        }
        options.keepAliveInterval = transport_data->keepAliveValue;
        options.useCleanSession = false;
        options.qualityOfServiceValue = DELIVER_AT_LEAST_ONCE;

        if (GetTransportProviderIfNecessary(transport_data) == 0)
        {
            if (mqtt_client_connect(transport_data->mqttClient, transport_data->xioTransport, &options) != 0)
            {
                LogError("failure connecting to address %s.", STRING_c_str(transport_data->hostAddress));
                result = __FAILURE__;
            }
            else
            {
                (void)tickcounter_get_current_ms(transport_data->msgTickCounter, &transport_data->mqtt_connect_time);
                result = 0;
            }
        }
        else
        {
            result = __FAILURE__;
        }
            
        if (sasToken != NULL)
        {
            free(sasToken);
        }
    }
    return result;
}

static int InitializeConnection(PMQTTTRANSPORT_HANDLE_DATA transport_data)
{
    int result = 0;

    // Make sure we're not destroying the object
    if (!transport_data->isDestroyCalled)
    {
        RETRY_ACTION retry_action = RETRY_ACTION_RETRY_LATER;

        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_007: [ IoTHubTransport_MQTT_Common_DoWork shall try to reconnect according to the current retry policy set ]
        if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_NOT_CONNECTED && transport_data->isRecoverableError && 
            (retry_control_should_retry(transport_data->retry_control_handle, &retry_action) != 0 || retry_action == RETRY_ACTION_RETRY_NOW))
        {
            // Note: in case retry_control_should_retry fails, the reconnection shall be attempted anyway (defaulting to policy IOTHUB_CLIENT_RETRY_IMMEDIATE).

            if (tickcounter_get_current_ms(transport_data->msgTickCounter, &transport_data->connectTick) != 0)
            {
                transport_data->connectFailCount++;
                result = __FAILURE__;
            }
            else
            {
                if (SendMqttConnectMsg(transport_data) != 0)
                {
                    transport_data->connectFailCount++;
                    result = __FAILURE__;
                }
                else
                {
                    transport_data->mqttClientStatus = MQTT_CLIENT_STATUS_CONNECTING;
                    transport_data->connectFailCount = 0;
                    result = 0;
                }
            }
        }
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_001: [ IoTHubTransport_MQTT_Common_DoWork shall trigger reconnection if the mqtt_client_connect does not complete within `keepalive` seconds]
        else if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_CONNECTING)
        {
            tickcounter_ms_t current_time;
            if (tickcounter_get_current_ms(transport_data->msgTickCounter, &current_time) != 0)
            {
                LogError("failed verifying MQTT_CLIENT_STATUS_CONNECTING timeout");
                result = __FAILURE__;
            }
            else if ((current_time - transport_data->mqtt_connect_time) / 1000 > transport_data->keepAliveValue) 
            {
                LogError("mqtt_client timed out waiting for CONNACK");
                DisconnectFromClient(transport_data);
                result = 0;
            }
        }
        else if (transport_data->mqttClientStatus == MQTT_CLIENT_STATUS_CONNECTED)
        {
            // We are connected and not being closed, so does SAS need to reconnect?
            tickcounter_ms_t current_time;
            if (tickcounter_get_current_ms(transport_data->msgTickCounter, &current_time) != 0)
            {
                transport_data->connectFailCount++;
                result = __FAILURE__;
            }
            else
            {
                if ((current_time - transport_data->mqtt_connect_time) / 1000 > (SAS_TOKEN_DEFAULT_LIFETIME * SAS_REFRESH_MULTIPLIER))
                {
                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_058: [ If the sas token has timed out IoTHubTransport_MQTT_Common_DoWork shall disconnect from the mqtt client and destroy the transport information and wait for reconnect. ] */
                    DisconnectFromClient(transport_data);
                    
                    IoTHubClient_LL_ConnectionStatusCallBack(transport_data->llClientHandle, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN);
                    
                    transport_data->device_twin_get_sent = false;
                    UpdateSubscribeFlag(transport_data);
                    result = __FAILURE__;
                }
            }
        }
    }
    return result;
}

static STRING_HANDLE buildConfigForUsername(const IOTHUB_CLIENT_CONFIG* upperConfig)
{
    return STRING_construct_sprintf("%s.%s/%s/api-version=%s&DeviceClientType=", upperConfig->iotHubName, upperConfig->iotHubSuffix, upperConfig->deviceId, IOTHUB_API_VERSION);
}

static PMQTTTRANSPORT_HANDLE_DATA InitializeTransportHandleData(const IOTHUB_CLIENT_CONFIG* upperConfig, PDLIST_ENTRY waitingToSend, IOTHUB_AUTHORIZATION_HANDLE auth_module)
{
    PMQTTTRANSPORT_HANDLE_DATA state = (PMQTTTRANSPORT_HANDLE_DATA)malloc(sizeof(MQTTTRANSPORT_HANDLE_DATA));
    if (state == NULL)
    {
        LogError("Could not create MQTT transport state. Memory allocation failed.");
    }
    else
    {
        memset(state, 0, sizeof(MQTTTRANSPORT_HANDLE_DATA) );
        if ((state->msgTickCounter = tickcounter_create()) == NULL)
        {
            LogError("Invalid Argument: iotHubName is empty");
            free(state);
            state = NULL;
        }
        // Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_09_005: [ MQTT transport shall use EXPONENTIAL_WITH_BACK_OFF as default retry policy ]
        else if ((state->retry_control_handle = retry_control_create(DEFAULT_RETRY_POLICY, DEFAULT_RETRY_TIMEOUT_IN_SECONDS)) == NULL)
        {
            LogError("Failed creating default retry control");
            tickcounter_destroy(state->msgTickCounter);
            free(state);
            state = NULL;
        }
        else if ((state->device_id = STRING_construct(upperConfig->deviceId)) == NULL)
        {
            LogError("failure constructing device_id.");
            retry_control_destroy(state->retry_control_handle);
            tickcounter_destroy(state->msgTickCounter);
            free(state);
            state = NULL;
        }
        else if ((state->devicesPath = STRING_construct_sprintf("%s.%s/devices/%s", upperConfig->iotHubName, upperConfig->iotHubSuffix, upperConfig->deviceId)) == NULL)
        {
            STRING_delete(state->device_id);
            retry_control_destroy(state->retry_control_handle);
            tickcounter_destroy(state->msgTickCounter);
            free(state);
            state = NULL;
        }
        else
        {
            if ( (state->topic_MqttEvent = STRING_construct_sprintf(TOPIC_DEVICE_DEVICE, upperConfig->deviceId) ) == NULL)
            {
                LogError("Could not create topic_MqttEvent for MQTT");
                STRING_delete(state->devicesPath);
                STRING_delete(state->device_id);
                retry_control_destroy(state->retry_control_handle);
                tickcounter_destroy(state->msgTickCounter);
                free(state);
                state = NULL;
            }
            else
            {
                state->mqttClient = mqtt_client_init(mqtt_notification_callback, mqtt_operation_complete_callback, state, mqtt_error_callback, state);
                if (state->mqttClient == NULL)
                {
                    LogError("failure initializing mqtt client.");
                    STRING_delete(state->devicesPath);
                    STRING_delete(state->topic_MqttEvent);
                    STRING_delete(state->device_id);
                    retry_control_destroy(state->retry_control_handle);
                    tickcounter_destroy(state->msgTickCounter);
                    free(state);
                    state = NULL;
                }
                else
                {
                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_008: [If the upperConfig contains a valid protocolGatewayHostName value the this shall be used for the hostname, otherwise the hostname shall be constructed using the iothubname and iothubSuffix.] */
                    if (upperConfig->protocolGatewayHostName == NULL)
                    {
                         state->hostAddress = STRING_construct_sprintf("%s.%s", upperConfig->iotHubName, upperConfig->iotHubSuffix);
                    }
                    else
                    {
                        state->hostAddress = STRING_construct(upperConfig->protocolGatewayHostName);
                    }
                
                    if (state->hostAddress == NULL)
                    {
                        LogError("failure constructing host address.");
                        mqtt_client_deinit(state->mqttClient);
                        STRING_delete(state->devicesPath);
                        STRING_delete(state->topic_MqttEvent);
                        STRING_delete(state->device_id);
                        retry_control_destroy(state->retry_control_handle);
                        tickcounter_destroy(state->msgTickCounter);
                        free(state);
                        state = NULL;
                    }
                    else if ((state->configPassedThroughUsername = buildConfigForUsername(upperConfig)) == NULL)
                    {
                        STRING_delete(state->hostAddress);
                        mqtt_client_deinit(state->mqttClient);
                        STRING_delete(state->devicesPath);
                        STRING_delete(state->topic_MqttEvent);
                        STRING_delete(state->device_id);
                        retry_control_destroy(state->retry_control_handle);
                        tickcounter_destroy(state->msgTickCounter);
                        free(state);
                        state = NULL;
                    }
                    else
                    {
                        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_010: [IoTHubTransport_MQTT_Common_Create shall allocate memory to save its internal state where all topics, hostname, device_id, device_key, sasTokenSr and client handle shall be saved.] */
                        DList_InitializeListHead(&(state->telemetry_waitingForAck));
                        DList_InitializeListHead(&(state->ack_waiting_queue));
                        state->isDestroyCalled = false;
                        state->isRegistered = false;
                        state->mqttClientStatus = MQTT_CLIENT_STATUS_NOT_CONNECTED;
                        state->device_twin_get_sent = false;
                        state->isRecoverableError = true;
                        state->packetId = 1;
                        state->llClientHandle = NULL;
                        state->xioTransport = NULL;
                        state->portNum = 0;
                        state->waitingToSend = waitingToSend;
                        state->currPacketState = CONNECT_TYPE;
                        state->keepAliveValue = DEFAULT_MQTT_KEEPALIVE;
                        state->connectFailCount = 0;
                        state->connectTick = 0;
                        state->topic_MqttMessage = NULL;
                        state->topic_GetState = NULL;
                        state->topic_NotifyState = NULL;
                        state->topics_ToSubscribe = UNSUBSCRIBE_FROM_TOPIC;
                        state->topic_DeviceMethods = NULL;
                        state->log_trace = state->raw_trace = false;
                        srand((unsigned int)get_time(NULL));
                        state->authorization_module = auth_module;
                        state->isProductInfoSet = false;
                    }
                }
            }
        }
    }
    return state;
}

TRANSPORT_LL_HANDLE IoTHubTransport_MQTT_Common_Create(const IOTHUBTRANSPORT_CONFIG* config, MQTT_GET_IO_TRANSPORT get_io_transport)
{
    PMQTTTRANSPORT_HANDLE_DATA result;
    size_t deviceIdSize;

    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_001: [If parameter config is NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_041: [ if get_io_transport is NULL then IoTHubTransport_MQTT_Common_Create shall return NULL. ] */
    if (config == NULL || get_io_transport == NULL)
    {
        LogError("Invalid Argument: Config Parameter is NULL.");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [If the parameter config's variables upperConfig, auth_module_handle or waitingToSend are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if (config->auth_module_handle == NULL)
    {
        LogError("Invalid Argument: auth_module_handle is NULL)");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [If the parameter config's variables upperConfig or waitingToSend are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, both deviceKey and deviceSasToken, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_03_003: [If both deviceKey & deviceSasToken fields are NOT NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if (config->upperConfig == NULL ||
             config->upperConfig->protocol == NULL || 
             config->upperConfig->deviceId == NULL || 
             ((config->upperConfig->deviceKey != NULL) && (config->upperConfig->deviceSasToken != NULL)) ||
             //is_key_validate(config) != 0 ||
             config->upperConfig->iotHubName == NULL || 
             config->upperConfig->iotHubSuffix == NULL)
    {
        LogError("Invalid Argument: upperConfig structure contains an invalid parameter");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_002: [If the parameter config's variables upperConfig, auth_module_handle or waitingToSend are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if (config->waitingToSend == NULL)
    {
        LogError("Invalid Argument: waitingToSend is NULL)");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_006: [If the upperConfig's variables deviceId is an empty strings or length is greater then 128 then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if ( ( (deviceIdSize = strlen(config->upperConfig->deviceId)) > 128U) || (deviceIdSize == 0) )
    {
        LogError("Invalid Argument: DeviceId is of an invalid size");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, both deviceKey and deviceSasToken, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if ((config->upperConfig->deviceKey != NULL) && (strlen(config->upperConfig->deviceKey) == 0))
    {
        LogError("Invalid Argument: deviceKey is empty");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, both deviceKey and deviceSasToken, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if ((config->upperConfig->deviceSasToken != NULL) && (strlen(config->upperConfig->deviceSasToken) == 0))
    {
        LogError("Invalid Argument: deviceSasToken is empty");
        result = NULL;
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_003: [If the upperConfig's variables deviceId, deviceKey, iotHubName, protocol, or iotHubSuffix are NULL then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    else if (strlen(config->upperConfig->iotHubName) == 0)
    {
        LogError("Invalid Argument: iotHubName is empty");
        result = NULL;
    }
    else
    {
        result = InitializeTransportHandleData(config->upperConfig, config->waitingToSend, config->auth_module_handle);
        if (result != NULL)
        {
            result->get_io_transport = get_io_transport;
            result->http_proxy_hostname = NULL;
            result->http_proxy_username = NULL;
            result->http_proxy_password = NULL;
        }
    }
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_009: [If any error is encountered then IoTHubTransport_MQTT_Common_Create shall return NULL.] */
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_011: [On Success IoTHubTransport_MQTT_Common_Create shall return a non-NULL value.] */
    return result;
}

void IoTHubTransport_MQTT_Common_Destroy(TRANSPORT_LL_HANDLE handle)
{
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_012: [IoTHubTransport_MQTT_Common_Destroy shall do nothing if parameter handle is NULL.] */
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data != NULL)
    {
        transport_data->isDestroyCalled = true;

        DisconnectFromClient(transport_data);

        //Empty the Waiting for Ack Messages.
        while (!DList_IsListEmpty(&transport_data->telemetry_waitingForAck))
        {
            PDLIST_ENTRY currentEntry = DList_RemoveHeadList(&transport_data->telemetry_waitingForAck);
            MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry = containingRecord(currentEntry, MQTT_MESSAGE_DETAILS_LIST, entry);
            sendMsgComplete(mqttMsgEntry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY);
            free(mqttMsgEntry);
        }
        while (!DList_IsListEmpty(&transport_data->ack_waiting_queue))
        {
            PDLIST_ENTRY currentEntry = DList_RemoveHeadList(&transport_data->ack_waiting_queue);
            MQTT_DEVICE_TWIN_ITEM* mqtt_device_twin = containingRecord(currentEntry, MQTT_DEVICE_TWIN_ITEM, entry);
            IoTHubClient_LL_ReportedStateComplete(transport_data->llClientHandle, mqtt_device_twin->iothub_msg_id, STATUS_CODE_TIMEOUT_VALUE);
            free(mqtt_device_twin);
        }

        STRING_delete(transport_data->devicesPath);

        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_014: [IoTHubTransport_MQTT_Common_Destroy shall free all the resources currently in use.] */
        mqtt_client_deinit(transport_data->mqttClient);
        retry_control_destroy(transport_data->retry_control_handle);
        STRING_delete(transport_data->topic_MqttEvent);
        STRING_delete(transport_data->topic_MqttMessage);
        STRING_delete(transport_data->device_id);
        STRING_delete(transport_data->hostAddress);
        STRING_delete(transport_data->configPassedThroughUsername);
        STRING_delete(transport_data->topic_GetState);
        STRING_delete(transport_data->topic_NotifyState);
        STRING_delete(transport_data->topic_DeviceMethods);

        set_saved_tls_options(transport_data, NULL);

        tickcounter_destroy(transport_data->msgTickCounter);
        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_012: [ `IoTHubTransport_MQTT_Common_Destroy` shall free the stored proxy options. ]*/
        free_proxy_data(transport_data);
        free(transport_data);
    }
}

int IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data == NULL)
    {
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_042: [If the parameter handle is NULL than IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
        LogError("Invalid handle parameter. NULL.");
        result = __FAILURE__;
    }
    else
    {
        if (transport_data->topic_GetState == NULL)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_044: [IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall construct the get state topic string and the notify state topic string.] */
            transport_data->topic_GetState = STRING_construct(TOPIC_GET_DESIRED_STATE);
            if (transport_data->topic_GetState == NULL)
            {
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_046: [Upon failure IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return a non-zero value.] */
                LogError("Failure: unable constructing reported state topic");
                result = __FAILURE__;
            }
            else
            {
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_047: [On success IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return 0.] */
                transport_data->topics_ToSubscribe |= SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
                result = 0;
            }
        }
        else
        {
            result = 0;
        }
        if (result == 0 && transport_data->topic_NotifyState == NULL)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_044: [`IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin` shall construct the get state topic string and the notify state topic string.] */
            transport_data->topic_NotifyState = STRING_construct(TOPIC_NOTIFICATION_STATE);
            if (transport_data->topic_NotifyState == NULL)
            {
                LogError("Failure: unable constructing notify state topic");
                result = __FAILURE__;
            }
            else
            {
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_047: [On success IoTHubTransport_MQTT_Common_Subscribe_DeviceTwin shall return 0.] */
                transport_data->topics_ToSubscribe |= SUBSCRIBE_NOTIFICATION_STATE_TOPIC;
                result = 0;
            }
        }
        if (result == 0)
        {
            if (transport_data->currPacketState != CONNACK_TYPE &&
                transport_data->currPacketState != CONNECT_TYPE &&
                transport_data->currPacketState != DISCONNECT_TYPE &&
                transport_data->currPacketState != PACKET_TYPE_ERROR)
            {
                transport_data->currPacketState = SUBSCRIBE_TYPE;
            }
        }
    }
    return result;
}

void IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin(IOTHUB_DEVICE_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_048: [If the parameter handle is NULL than IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin shall do nothing.] */
    if (transport_data != NULL)
    {
        if (transport_data->topic_GetState != NULL)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_049: [If subscribe_state is set to IOTHUB_DEVICE_TWIN_DESIRED_STATE then IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin shall unsubscribe from the topic_GetState to the mqtt client.] */
            transport_data->topics_ToSubscribe &= ~SUBSCRIBE_GET_REPORTED_STATE_TOPIC;
            STRING_delete(transport_data->topic_GetState);
            transport_data->topic_GetState = NULL;
        }
        if (transport_data->topic_NotifyState != NULL)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_050: [If subscribe_state is set to IOTHUB_DEVICE_TWIN_NOTIFICATION_STATE then IoTHubTransport_MQTT_Common_Unsubscribe_DeviceTwin shall unsubscribe from the topic_NotifyState to the mqtt client.] */
            transport_data->topics_ToSubscribe &= ~SUBSCRIBE_NOTIFICATION_STATE_TOPIC;
            STRING_delete(transport_data->topic_NotifyState);
            transport_data->topic_NotifyState = NULL;
        }
    }
    else
    {
        LogError("Invalid argument to unsubscribe (handle is NULL).");
    }
}

int IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;

    if (transport_data == NULL)
    {
        /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_001 : [If the parameter handle is NULL than IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall return a non - zero value.]*/
        LogError("Invalid handle parameter. NULL.");
        result = __FAILURE__;
    }
    else
    {
        if (transport_data->topic_DeviceMethods == NULL)
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_004 : [IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall construct the DEVICE_METHOD topic string for subscribe.]*/
            transport_data->topic_DeviceMethods = STRING_construct(TOPIC_DEVICE_METHOD_SUBSCRIBE);
            if (transport_data->topic_DeviceMethods == NULL)
            {
                /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_006 : [Upon failure IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall return a non - zero value.]*/
                LogError("Failure: unable constructing device method subscribe topic");
                result = __FAILURE__;
            }
            else
            {
                /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_003 : [IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall set the signaling flag for DEVICE_METHOD topic for the receiver's topic list. ]*/
                transport_data->topics_ToSubscribe |= SUBSCRIBE_DEVICE_METHOD_TOPIC;
                result = 0;
            }
        }
        else
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_002 : [If the MQTT transport has been previously subscribed to DEVICE_METHOD topic IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall do nothing and return 0.]*/
            result = 0;
        }

        if (result == 0)
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_005 : [IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall schedule the send of the subscription.]*/
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_007 : [On success IoTHubTransport_MQTT_Common_Subscribe_DeviceMethod shall return 0.]*/
            if (transport_data->currPacketState != CONNACK_TYPE &&
                transport_data->currPacketState != CONNECT_TYPE &&
                transport_data->currPacketState != DISCONNECT_TYPE &&
                transport_data->currPacketState != PACKET_TYPE_ERROR)
            {
                transport_data->currPacketState = SUBSCRIBE_TYPE;
            }
        }
    }
    return result;
}

void IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod(IOTHUB_DEVICE_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_008 : [If the parameter handle is NULL than IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall do nothing and return.]*/
    if (transport_data != NULL)
    {
        /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_009 : [If the MQTT transport has not been subscribed to DEVICE_METHOD topic IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall do nothing and return.]*/
        if (transport_data->topic_DeviceMethods != NULL)
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_010 : [IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall construct the DEVICE_METHOD topic string for unsubscribe.]*/
            const char* unsubscribe[1];
            unsubscribe[0] = STRING_c_str(transport_data->topic_DeviceMethods);

            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_011 : [IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall send the unsubscribe.]*/
            if (mqtt_client_unsubscribe(transport_data->mqttClient, get_next_packet_id(transport_data), unsubscribe, 1) != 0)
            {
                LogError("Failure calling mqtt_client_unsubscribe");
            }

            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_12_012 : [IoTHubTransport_MQTT_Common_Unsubscribe_DeviceMethod shall removes the signaling flag for DEVICE_METHOD topic from the receiver's topic list. ]*/
            STRING_delete(transport_data->topic_DeviceMethods);
            transport_data->topic_DeviceMethods = NULL;
            transport_data->topics_ToSubscribe &= ~SUBSCRIBE_DEVICE_METHOD_TOPIC;
        }
    }
    else
    {
        LogError("Invalid argument to unsubscribe (NULL).");
    }
}

int IoTHubTransport_MQTT_Common_DeviceMethod_Response(IOTHUB_DEVICE_HANDLE handle, METHOD_HANDLE methodId, const unsigned char* response, size_t respSize, int status)
{
    int result;
    MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)handle;
    if (transport_data != NULL)
    {
        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_042: [ IoTHubTransport_MQTT_Common_DeviceMethod_Response shall publish an mqtt message for the device method response. ] */
        DEVICE_METHOD_INFO* dev_method_info = (DEVICE_METHOD_INFO*)methodId;
        if (dev_method_info == NULL)
        {
            LogError("Failure: DEVICE_METHOD_INFO was NULL");
            result = __FAILURE__;
        }
        else
        {
            if (publish_device_method_message(transport_data, status, dev_method_info->request_id, response, respSize) != 0)
            {
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_051: [ If any error is encountered, IoTHubTransport_MQTT_Common_DeviceMethod_Response shall return a non-zero value. ] */
                LogError("Failure: publishing device method response");
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
            STRING_delete(dev_method_info->request_id);
            free(dev_method_info);
        }
    }
    else
    {
        /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_041: [ If the parameter handle is NULL than IoTHubTransport_MQTT_Common_DeviceMethod_Response shall return a non-zero value. ] */
        result = __FAILURE__;
        LogError("Failure: invalid IOTHUB_DEVICE_HANDLE parameter specified");
    }
    return result;
}

int IoTHubTransport_MQTT_Common_Subscribe(IOTHUB_DEVICE_HANDLE handle)
{
    int result;
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data == NULL)
    {
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_015: [If parameter handle is NULL than IoTHubTransport_MQTT_Common_Subscribe shall return a non-zero value.] */
        LogError("Invalid handle parameter. NULL.");
        result = __FAILURE__;
    }
    else
    {
        /* Code_SRS_IOTHUB_MQTT_TRANSPORT_07_016: [IoTHubTransport_MQTT_Common_Subscribe shall set a flag to enable mqtt_client_subscribe to be called to subscribe to the Message Topic.] */
        transport_data->topic_MqttMessage = STRING_construct_sprintf(TOPIC_DEVICE_MSG, STRING_c_str(transport_data->device_id) );
        if (transport_data->topic_MqttMessage == NULL)
        {
            LogError("Failure constructing Message Topic");
            result = __FAILURE__;
        }
        else
        {
            transport_data->topics_ToSubscribe |= SUBSCRIBE_TELEMETRY_TOPIC;
            /* Code_SRS_IOTHUB_MQTT_TRANSPORT_07_035: [If current packet state is not CONNACT, DISCONNECT_TYPE, or PACKET_TYPE_ERROR then IoTHubTransport_MQTT_Common_Subscribe shall set the packet state to SUBSCRIBE_TYPE.]*/
            if (transport_data->currPacketState != CONNACK_TYPE &&
                transport_data->currPacketState != CONNECT_TYPE &&
                transport_data->currPacketState != DISCONNECT_TYPE &&
                transport_data->currPacketState != PACKET_TYPE_ERROR)
            {
                transport_data->currPacketState = SUBSCRIBE_TYPE;
            }
            result = 0;
        }
    }
    return result;
}

void IoTHubTransport_MQTT_Common_Unsubscribe(IOTHUB_DEVICE_HANDLE handle)
{
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_019: [If parameter handle is NULL then IoTHubTransport_MQTT_Common_Unsubscribe shall do nothing.] */
    if (transport_data != NULL)
    {
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_020: [IoTHubTransport_MQTT_Common_Unsubscribe shall call mqtt_client_unsubscribe to unsubscribe the mqtt message topic.] */
        const char* unsubscribe[1];
        unsubscribe[0] = STRING_c_str(transport_data->topic_MqttMessage);
        if (mqtt_client_unsubscribe(transport_data->mqttClient, get_next_packet_id(transport_data), unsubscribe, 1) != 0)
        {
            LogError("Failure calling mqtt_client_unsubscribe");
        }
        STRING_delete(transport_data->topic_MqttMessage);
        transport_data->topic_MqttMessage = NULL;
        transport_data->topics_ToSubscribe &= ~SUBSCRIBE_TELEMETRY_TOPIC;
    }
    else
    {
        LogError("Invalid argument to unsubscribe (NULL).");
    }
}

IOTHUB_PROCESS_ITEM_RESULT IoTHubTransport_MQTT_Common_ProcessItem(TRANSPORT_LL_HANDLE handle, IOTHUB_IDENTITY_TYPE item_type, IOTHUB_IDENTITY_INFO* iothub_item)
{
    IOTHUB_PROCESS_ITEM_RESULT result;
    /* Codes_SRS_IOTHUBCLIENT_LL_07_001: [ If handle or iothub_item are NULL then IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_ERROR. ]*/
    if (handle == NULL || iothub_item == NULL)
    {
        LogError("Invalid handle parameter iothub_item=%p", iothub_item);
        result = IOTHUB_PROCESS_ERROR;
    }
    else
    {
        PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;

        if (transport_data->currPacketState == PUBLISH_TYPE)
        {
            if (item_type == IOTHUB_TYPE_DEVICE_TWIN)
            {
                MQTT_DEVICE_TWIN_ITEM* mqtt_info = (MQTT_DEVICE_TWIN_ITEM*)malloc(sizeof(MQTT_DEVICE_TWIN_ITEM));
                if (mqtt_info == NULL)
                {
                    /* Codes_SRS_IOTHUBCLIENT_LL_07_004: [ If any errors are encountered IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_ERROR. ]*/
                    result = IOTHUB_PROCESS_ERROR;
                }
                else
                {
                    /*Codes_SRS_IOTHUBCLIENT_LL_07_003: [ IoTHubTransport_MQTT_Common_ProcessItem shall publish a message to the mqtt protocol with the message topic for the message type.]*/
                    mqtt_info->iothub_type = item_type;
                    mqtt_info->iothub_msg_id = iothub_item->device_twin->item_id;
                    mqtt_info->retryCount = 0;
                    
                    /* Codes_SRS_IOTHUBCLIENT_LL_07_005: [ If successful IoTHubTransport_MQTT_Common_ProcessItem shall add mqtt info structure acknowledgement queue. ] */
                    DList_InsertTailList(&transport_data->ack_waiting_queue, &mqtt_info->entry);

                    if (publish_device_twin_message(transport_data, iothub_item->device_twin, mqtt_info) != 0)
                    {
                        DList_RemoveEntryList(&mqtt_info->entry);

                        free(mqtt_info);
                        /* Codes_SRS_IOTHUBCLIENT_LL_07_004: [ If any errors are encountered IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_ERROR. ]*/
                        result = IOTHUB_PROCESS_ERROR;
                    }
                    else
                    {
                        result = IOTHUB_PROCESS_OK;
                    }
                }
            }
            else
            {
                /* Codes_SRS_IOTHUBCLIENT_LL_07_006: [ If the item_type is not a supported type IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_CONTINUE. ]*/
                result = IOTHUB_PROCESS_CONTINUE;
            }
        }
        else
        {
            /* Codes_SRS_IOTHUBCLIENT_LL_07_002: [ If the mqtt is not ready to publish messages IoTHubTransport_MQTT_Common_ProcessItem shall return IOTHUB_PROCESS_NOT_CONNECTED. ] */
            result = IOTHUB_PROCESS_NOT_CONNECTED;
        }
    }
    return result;
}

/* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_054: [ IoTHubTransport_MQTT_Common_DoWork shall subscribe to the Notification and get_state Topics if they are defined. ] */
void IoTHubTransport_MQTT_Common_DoWork(TRANSPORT_LL_HANDLE handle, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
{
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_026: [IoTHubTransport_MQTT_Common_DoWork shall do nothing if parameter handle and/or iotHubClientHandle is NULL.] */
    PMQTTTRANSPORT_HANDLE_DATA transport_data = (PMQTTTRANSPORT_HANDLE_DATA)handle;
    if (transport_data != NULL && iotHubClientHandle != NULL)
    {
        transport_data->llClientHandle = iotHubClientHandle;

        if (InitializeConnection(transport_data) != 0)
        {
            // Don't want to flood the logs with failures here
        }
        else
        {
            if (transport_data->currPacketState == CONNACK_TYPE || transport_data->currPacketState == SUBSCRIBE_TYPE)
            {
                SubscribeToMqttProtocol(transport_data);
            }
            else if (transport_data->currPacketState == SUBACK_TYPE)
            {
                if ((transport_data->topic_NotifyState != NULL || transport_data->topic_GetState != NULL) &&
                    !transport_data->device_twin_get_sent)
                {
                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_055: [ IoTHubTransport_MQTT_Common_DoWork shall send a device twin get property message upon successfully retrieving a SUBACK on device twin topics. ] */
                    if (publish_device_twin_get_message(transport_data) == 0)
                    {
                        transport_data->device_twin_get_sent = true;
                    }
                    else
                    {
                        LogError("Failure: sending device twin get property command.");
                    }
                }
                // Publish can be called now
                transport_data->currPacketState = PUBLISH_TYPE;
            }
            else if (transport_data->currPacketState == PUBLISH_TYPE)
            {
                PDLIST_ENTRY currentListEntry = transport_data->telemetry_waitingForAck.Flink;
                while (currentListEntry != &transport_data->telemetry_waitingForAck)
                {
                    tickcounter_ms_t current_ms;
                    MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry = containingRecord(currentListEntry, MQTT_MESSAGE_DETAILS_LIST, entry);
                    DLIST_ENTRY nextListEntry;
                    nextListEntry.Flink = currentListEntry->Flink;

                    (void)tickcounter_get_current_ms(transport_data->msgTickCounter, &current_ms);
                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_033: [IoTHubTransport_MQTT_Common_DoWork shall iterate through the Waiting Acknowledge messages looking for any message that has been waiting longer than 2 min.]*/
                    if (((current_ms - mqttMsgEntry->msgPublishTime) / 1000) > RESEND_TIMEOUT_VALUE_MIN)
                    {
                        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_034: [If IoTHubTransport_MQTT_Common_DoWork has resent the message two times then it shall fail the message and reconnect to IoTHub ... ] */
                        if (mqttMsgEntry->retryCount >= MAX_SEND_RECOUNT_LIMIT)
                        {
                            PDLIST_ENTRY current_entry;
                            (void)DList_RemoveEntryList(currentListEntry);
                            sendMsgComplete(mqttMsgEntry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT);
                            free(mqttMsgEntry);
                                                        
                            /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_07_057: [ ... then go through all the rest of the waiting messages and reset the retryCount on the message. ]*/
                            current_entry = transport_data->telemetry_waitingForAck.Flink;
                            while (current_entry != &transport_data->telemetry_waitingForAck)
                            {
                                MQTT_MESSAGE_DETAILS_LIST* msg_reset_entry;
                                msg_reset_entry = containingRecord(current_entry, MQTT_MESSAGE_DETAILS_LIST, entry);
                                msg_reset_entry->retryCount = 0;
                                current_entry = current_entry->Flink;
                            }

                            transport_data->currPacketState = PACKET_TYPE_ERROR;
                            transport_data->device_twin_get_sent = false;
                            DisconnectFromClient(transport_data);
                            return;
                        }
                        else
                        {
                            size_t messageLength;
                            const unsigned char* messagePayload = RetrieveMessagePayload(mqttMsgEntry->iotHubMessageEntry->messageHandle, &messageLength);
                            if (messageLength == 0 || messagePayload == NULL)
                            {
                                LogError("Failure from creating Message IoTHubMessage_GetData");
                            }
                            else
                            {
                                if (publish_mqtt_telemetry_msg(transport_data, mqttMsgEntry, messagePayload, messageLength) != 0)
                                {
                                    (void)DList_RemoveEntryList(currentListEntry);
                                    sendMsgComplete(mqttMsgEntry->iotHubMessageEntry, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
                                    free(mqttMsgEntry);
                                }
                            }
                        }
                    }
                    currentListEntry = nextListEntry.Flink;
                }

                currentListEntry = transport_data->waitingToSend->Flink;
                /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_027: [IoTHubTransport_MQTT_Common_DoWork shall inspect the "waitingToSend" DLIST passed in config structure.] */
                while (currentListEntry != transport_data->waitingToSend)
                {
                    IOTHUB_MESSAGE_LIST* iothubMsgList = containingRecord(currentListEntry, IOTHUB_MESSAGE_LIST, entry);
                    DLIST_ENTRY savedFromCurrentListEntry;
                    savedFromCurrentListEntry.Flink = currentListEntry->Flink;

                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_027: [IoTHubTransport_MQTT_Common_DoWork shall inspect the "waitingToSend" DLIST passed in config structure.] */
                    size_t messageLength;
                    const unsigned char* messagePayload = RetrieveMessagePayload(iothubMsgList->messageHandle, &messageLength);
                    if (messageLength == 0 || messagePayload == NULL)
                    {
                        LogError("Failure result from IoTHubMessage_GetData");
                    }
                    else
                    {
                        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_029: [IoTHubTransport_MQTT_Common_DoWork shall create a MQTT_MESSAGE_HANDLE and pass this to a call to mqtt_client_publish.] */
                        MQTT_MESSAGE_DETAILS_LIST* mqttMsgEntry = (MQTT_MESSAGE_DETAILS_LIST*)malloc(sizeof(MQTT_MESSAGE_DETAILS_LIST));
                        if (mqttMsgEntry == NULL)
                        {
                            LogError("Allocation Error: Failure allocating MQTT Message Detail List.");
                        }
                        else
                        {
                            mqttMsgEntry->retryCount = 0;
                            mqttMsgEntry->iotHubMessageEntry = iothubMsgList;
                            mqttMsgEntry->packet_id = get_next_packet_id(transport_data);
                            if (publish_mqtt_telemetry_msg(transport_data, mqttMsgEntry, messagePayload, messageLength) != 0)
                            {
                                (void)(DList_RemoveEntryList(currentListEntry));
                                sendMsgComplete(iothubMsgList, transport_data, IOTHUB_CLIENT_CONFIRMATION_ERROR);
                                free(mqttMsgEntry);
                            }
                            else
                            {
                                (void)(DList_RemoveEntryList(currentListEntry));
                                DList_InsertTailList(&(transport_data->telemetry_waitingForAck), &(mqttMsgEntry->entry));
                            }
                        }
                    }
                    currentListEntry = savedFromCurrentListEntry.Flink;
                }
            }
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_030: [IoTHubTransport_MQTT_Common_DoWork shall call mqtt_client_dowork everytime it is called if it is connected.] */
            mqtt_client_dowork(transport_data->mqttClient);
        }
    }
}

IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_GetSendStatus(IOTHUB_DEVICE_HANDLE handle, IOTHUB_CLIENT_STATUS *iotHubClientStatus)
{
    IOTHUB_CLIENT_RESULT result;

    if (handle == NULL || iotHubClientStatus == NULL)
    {
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_023: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_INVALID_ARG if called with NULL parameter.] */
        LogError("invalid arument.");
        result = IOTHUB_CLIENT_INVALID_ARG;
    }
    else
    {
        MQTTTRANSPORT_HANDLE_DATA* handleData = (MQTTTRANSPORT_HANDLE_DATA*)handle;
        if (!DList_IsListEmpty(handleData->waitingToSend) || !DList_IsListEmpty(&(handleData->telemetry_waitingForAck)))
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_025: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_BUSY if there are currently event items to be sent or being sent.] */
            *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_BUSY;
        }
        else
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_024: [IoTHubTransport_MQTT_Common_GetSendStatus shall return IOTHUB_CLIENT_OK and status IOTHUB_CLIENT_SEND_STATUS_IDLE if there are currently no event items to be sent or being sent.] */
            *iotHubClientStatus = IOTHUB_CLIENT_SEND_STATUS_IDLE;
        }
        result = IOTHUB_CLIENT_OK;
    }
    return result;
}

IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_SetOption(TRANSPORT_LL_HANDLE handle, const char* option, const void* value)
{
    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_021: [If any parameter is NULL then IoTHubTransport_MQTT_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG.] */
    IOTHUB_CLIENT_RESULT result;
    if (
        (handle == NULL) ||
        (option == NULL) ||
        (value == NULL)
        )
    {
        result = IOTHUB_CLIENT_INVALID_ARG;
        LogError("invalid parameter (NULL) passed to IoTHubTransport_MQTT_Common_SetOption.");
    }
    else
    {
        MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)handle;

        IOTHUB_CREDENTIAL_TYPE cred_type = IoTHubClient_Auth_Get_Credential_Type(transport_data->authorization_module);

        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_031: [If the option parameter is set to "logtrace" then the value shall be a bool_ptr and the value will determine if the mqtt client log is on or off.] */
        if (strcmp(OPTION_LOG_TRACE, option) == 0)
        {
            transport_data->log_trace = *((bool*)value);
            mqtt_client_set_trace(transport_data->mqttClient, transport_data->log_trace, transport_data->raw_trace);
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp("rawlogtrace", option) == 0)
        {
            transport_data->raw_trace = *((bool*)value);
            mqtt_client_set_trace(transport_data->mqttClient, transport_data->log_trace, transport_data->raw_trace);
            result = IOTHUB_CLIENT_OK;
        }
        else if (strcmp(OPTION_KEEP_ALIVE, option) == 0)
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_036: [If the option parameter is set to "keepalive" then the value shall be a int_ptr and the value will determine the mqtt keepalive time that is set for pings.] */
            int* keepAliveOption = (int*)value;
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_037 : [If the option parameter is set to supplied int_ptr keepalive is the same value as the existing keepalive then IoTHubTransport_MQTT_Common_SetOption shall do nothing.] */
            if (*keepAliveOption != transport_data->keepAliveValue)
            {
                transport_data->keepAliveValue = (uint16_t)(*keepAliveOption);
                if (transport_data->mqttClientStatus != MQTT_CLIENT_STATUS_NOT_CONNECTED)
                {
                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_038: [If the client is connected when the keepalive is set then IoTHubTransport_MQTT_Common_SetOption shall disconnect and reconnect with the specified keepalive value.] */
                    DisconnectFromClient(transport_data);
                }
            }
            result = IOTHUB_CLIENT_OK;
        }
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_039: [If the option parameter is set to "x509certificate" then the value shall be a const char of the certificate to be used for x509.] */
        else if ((strcmp(OPTION_X509_CERT, option) == 0) && (cred_type != IOTHUB_CREDENTIAL_TYPE_X509 && cred_type != IOTHUB_CREDENTIAL_TYPE_UNKNOWN))
        {
            LogError("x509certificate specified, but authentication method is not x509");
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
        /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_040: [If the option parameter is set to "x509privatekey" then the value shall be a const char of the RSA Private Key to be used for x509.] */
        else if ((strcmp(OPTION_X509_PRIVATE_KEY, option) == 0) && (cred_type != IOTHUB_CREDENTIAL_TYPE_X509 && cred_type != IOTHUB_CREDENTIAL_TYPE_UNKNOWN))
        {
            LogError("x509privatekey specified, but authentication method is not x509");
            result = IOTHUB_CLIENT_INVALID_ARG;
        }
        else if (strcmp(OPTION_HTTP_PROXY, option) == 0)
        {
            /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_001: [ If `option` is `proxy_data`, `value` shall be used as an `HTTP_PROXY_OPTIONS*`. ]*/
            HTTP_PROXY_OPTIONS* proxy_options = (HTTP_PROXY_OPTIONS*)value;

            if (transport_data->xioTransport != NULL)
            {
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_007: [ If the underlying IO has already been created, then `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
                LogError("Cannot set proxy option once the underlying IO is created");
                result = IOTHUB_CLIENT_ERROR;
            }
            else if (proxy_options->host_address == NULL)
            {
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_003: [ If `host_address` is NULL, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
                LogError("NULL host_address in proxy options");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_006: [ If only one of `username` and `password` is NULL, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_INVALID_ARG`. ]*/
            else if (((proxy_options->username == NULL) || (proxy_options->password == NULL)) &&
                (proxy_options->username != proxy_options->password))
            {
                LogError("Only one of username and password for proxy settings was NULL");
                result = IOTHUB_CLIENT_INVALID_ARG;
            }
            else
            {
                char* copied_proxy_hostname = NULL;
                char* copied_proxy_username = NULL;
                char* copied_proxy_password = NULL;

                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_002: [ The fields `host_address`, `port`, `username` and `password` shall be saved for later used (needed when creating the underlying IO to be used by the transport). ]*/
                transport_data->http_proxy_port = proxy_options->port;
                if (mallocAndStrcpy_s(&copied_proxy_hostname, proxy_options->host_address) != 0)
                {
                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_004: [ If copying `host_address`, `username` or `password` fails, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
                    LogError("Cannot copy HTTP proxy hostname");
                    result = IOTHUB_CLIENT_ERROR;
                }
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_005: [ `username` and `password` shall be allowed to be NULL. ]*/
                else if ((proxy_options->username != NULL) && (mallocAndStrcpy_s(&copied_proxy_username, proxy_options->username) != 0))
                {
                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_004: [ If copying `host_address`, `username` or `password` fails, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
                    free(copied_proxy_hostname);
                    LogError("Cannot copy HTTP proxy username");
                    result = IOTHUB_CLIENT_ERROR;
                }
                /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_005: [ `username` and `password` shall be allowed to be NULL. ]*/
                else if ((proxy_options->password != NULL) && (mallocAndStrcpy_s(&copied_proxy_password, proxy_options->password) != 0))
                {
                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_004: [ If copying `host_address`, `username` or `password` fails, `IoTHubTransport_MQTT_Common_SetOption` shall fail and return `IOTHUB_CLIENT_ERROR`. ]*/
                    if (copied_proxy_username != NULL)
                    {
                        free(copied_proxy_username);
                    }
                    free(copied_proxy_hostname);
                    LogError("Cannot copy HTTP proxy password");
                    result = IOTHUB_CLIENT_ERROR;
                }
                else
                {
                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_009: [ When setting the proxy options succeeds any previously saved proxy options shall be freed. ]*/
                    free_proxy_data(transport_data);

                    transport_data->http_proxy_hostname = copied_proxy_hostname;
                    transport_data->http_proxy_username = copied_proxy_username;
                    transport_data->http_proxy_password = copied_proxy_password;

                    /* Codes_SRS_IOTHUB_TRANSPORT_MQTT_COMMON_01_008: [ If setting the `proxy_data` option suceeds, `IoTHubTransport_MQTT_Common_SetOption` shall return `IOTHUB_CLIENT_OK` ]*/
                    result = IOTHUB_CLIENT_OK;
                }
            }
        }
        else
        {
            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_039: [If the option parameter is set to "x509certificate" then the value shall be a const char of the certificate to be used for x509.] */
            if ( ((strcmp(OPTION_X509_CERT, option) == 0) || (strcmp(OPTION_X509_PRIVATE_KEY, option) == 0)) && (cred_type != IOTHUB_CREDENTIAL_TYPE_X509) )
            {
                IoTHubClient_Auth_Set_x509_Type(transport_data->authorization_module, true);
            }

            /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_032: [IoTHubTransport_MQTT_Common_SetOption shall pass down the option to xio_setoption if the option parameter is not a known option string for the MQTT transport.] */
            if (GetTransportProviderIfNecessary(transport_data) == 0)
            {
                if (xio_setoption(transport_data->xioTransport, option, value) == 0)
                {
                    result = IOTHUB_CLIENT_OK;
                }
                else
                {
                    /* Codes_SRS_IOTHUB_MQTT_TRANSPORT_07_132: [IoTHubTransport_MQTT_Common_SetOption shall return IOTHUB_CLIENT_INVALID_ARG xio_setoption fails] */
                    result = IOTHUB_CLIENT_INVALID_ARG;
                }
            }
            else
            {
                result = IOTHUB_CLIENT_ERROR;
            }
        }
    }
    return result;
}

IOTHUB_DEVICE_HANDLE IoTHubTransport_MQTT_Common_Register(TRANSPORT_LL_HANDLE handle, const IOTHUB_DEVICE_CONFIG* device, IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, PDLIST_ENTRY waitingToSend)
{
    IOTHUB_DEVICE_HANDLE result = NULL;
    (void)iotHubClientHandle;

    // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_001: [ IoTHubTransport_MQTT_Common_Register shall return NULL if the TRANSPORT_LL_HANDLE is NULL.]
    // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_002: [ IoTHubTransport_MQTT_Common_Register shall return NULL if device or waitingToSend are NULL.]
    if ((handle == NULL) || (device == NULL) || (waitingToSend == NULL))
    {
        LogError("IoTHubTransport_MQTT_Common_Register: handle, device or waitingToSend is NULL.");
        result = NULL;
    }
    else
    {
        MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)handle;

        // Codes_SRS_IOTHUB_MQTT_TRANSPORT_03_001: [ IoTHubTransport_MQTT_Common_Register shall return NULL if deviceId, or both deviceKey and deviceSasToken are NULL.]
        if (device->deviceId == NULL)
        {
            LogError("IoTHubTransport_MQTT_Common_Register: deviceId is NULL.");
            result = NULL;
        }
        // Codes_SRS_IOTHUB_MQTT_TRANSPORT_03_002: [ IoTHubTransport_MQTT_Common_Register shall return NULL if both deviceKey and deviceSasToken are provided.]
        else if ((device->deviceKey != NULL) && (device->deviceSasToken != NULL))
        {
            LogError("IoTHubTransport_MQTT_Common_Register: Both deviceKey and deviceSasToken are defined. Only one can be used.");
            result = NULL;
        }
        else
        {
            // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_003: [ IoTHubTransport_MQTT_Common_Register shall return NULL if deviceId or deviceKey do not match the deviceId and deviceKey passed in during IoTHubTransport_MQTT_Common_Create.]
            if (strcmp(STRING_c_str(transport_data->device_id), device->deviceId) != 0)
            {
                LogError("IoTHubTransport_MQTT_Common_Register: deviceId does not match.");
                result = NULL;
            }
            else if (IoTHubClient_Auth_Get_Credential_Type(transport_data->authorization_module) == IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY && 
                (strcmp(IoTHubClient_Auth_Get_DeviceKey(transport_data->authorization_module), device->deviceKey) != 0))
            {
                LogError("IoTHubTransport_MQTT_Common_Register: deviceKey does not match.");
                result = NULL;
            }
            else
            {
                if (transport_data->isRegistered == true)
                {
                    LogError("Transport already has device registered by id: [%s]", device->deviceId);
                    result = NULL;
                }
                else
                {
                    transport_data->isRegistered = true;
                    // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_004: [ IoTHubTransport_MQTT_Common_Register shall return the TRANSPORT_LL_HANDLE as the IOTHUB_DEVICE_HANDLE. ]
                    result = (IOTHUB_DEVICE_HANDLE)handle;
                }
            }
        }
    }

    return result;
}

void IoTHubTransport_MQTT_Common_Unregister(IOTHUB_DEVICE_HANDLE deviceHandle)
{
    // Codes_SRS_IOTHUB_MQTT_TRANSPORT_17_005: [ If deviceHandle is NULL `IoTHubTransport_MQTT_Common_Unregister` shall do nothing. ]
    if (deviceHandle != NULL)
    {
        MQTTTRANSPORT_HANDLE_DATA* transport_data = (MQTTTRANSPORT_HANDLE_DATA*)deviceHandle;

        transport_data->isRegistered = false;
    }
}

STRING_HANDLE IoTHubTransport_MQTT_Common_GetHostname(TRANSPORT_LL_HANDLE handle)
{
    STRING_HANDLE result;
    /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_02_001: [ If handle is NULL then IoTHubTransport_MQTT_Common_GetHostname shall fail and return NULL. ]*/
    if (handle == NULL)
    {
        result = NULL;
    }
    /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_02_002: [ Otherwise IoTHubTransport_MQTT_Common_GetHostname shall return a non-NULL STRING_HANDLE containg the hostname. ]*/
    else if ((result = STRING_clone(((MQTTTRANSPORT_HANDLE_DATA*)(handle))->hostAddress)) == NULL)
    {
        LogError("Cannot provide the target host name (STRING_clone failed).");
    }

    return result;
}

IOTHUB_CLIENT_RESULT IoTHubTransport_MQTT_Common_SendMessageDisposition(MESSAGE_CALLBACK_INFO* message_data, IOTHUBMESSAGE_DISPOSITION_RESULT disposition)
{
    (void)disposition;

    IOTHUB_CLIENT_RESULT result;
    if (message_data)
    {
        if (message_data->messageHandle)
        {
            IoTHubMessage_Destroy(message_data->messageHandle);
            result = IOTHUB_CLIENT_OK;
        }
        else
        {
            /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_10_002: [If any of the messageData fields are NULL, IoTHubTransport_MQTT_Common_SendMessageDisposition shall fail and return IOTHUB_CLIENT_ERROR. ]*/
            LogError("message handle is NULL");
            result = IOTHUB_CLIENT_ERROR;
        }
        free(message_data);
    }
    else
    {
        /*Codes_SRS_IOTHUB_MQTT_TRANSPORT_10_001: [If messageData is NULL, IoTHubTransport_MQTT_Common_SendMessageDisposition shall fail and return IOTHUB_CLIENT_ERROR. ]*/
        LogError("message_data is NULL");
        result = IOTHUB_CLIENT_ERROR;
    }
    return result;
}


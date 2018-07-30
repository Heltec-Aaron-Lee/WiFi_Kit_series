// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "az_iot/c-utility/inc/azure_c_shared_utility/optimize_size.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/gballoc.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/platform.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/tickcounter.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/crt_abstractions.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/xlogging.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/strings.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/agenttime.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/threadapi.h"

#include "../inc/azure_umqtt_c/mqtt_client.h"
#include "../inc/azure_umqtt_c/mqtt_codec.h"
#include <inttypes.h>

#define KEEP_ALIVE_BUFFER_SEC           10
#define VARIABLE_HEADER_OFFSET          2
#define RETAIN_FLAG_MASK                0x1
#define QOS_LEAST_ONCE_FLAG_MASK        0x2
#define QOS_EXACTLY_ONCE_FLAG_MASK      0x4
#define DUPLICATE_FLAG_MASK             0x8
#define CONNECT_PACKET_MASK             0xf0
#define TIME_MAX_BUFFER                 16
#define DEFAULT_MAX_PING_RESPONSE_TIME  80  // % of time to send pings
#define MAX_CLOSE_RETRIES               10

static const char* TRUE_CONST = "true";
static const char* FALSE_CONST = "false";

DEFINE_ENUM_STRINGS(QOS_VALUE, QOS_VALUE_VALUES);

typedef struct MQTT_CLIENT_TAG
{
    XIO_HANDLE xioHandle;
    MQTTCODEC_HANDLE codec_handle;
    CONTROL_PACKET_TYPE packetState;
    TICK_COUNTER_HANDLE packetTickCntr;
    tickcounter_ms_t packetSendTimeMs;
    ON_MQTT_OPERATION_CALLBACK fnOperationCallback;
    ON_MQTT_MESSAGE_RECV_CALLBACK fnMessageRecv;
    void* ctx;
    ON_MQTT_ERROR_CALLBACK fnOnErrorCallBack;
    void* errorCBCtx;
    ON_MQTT_DISCONNECTED_CALLBACK disconnect_cb;
    void* disconnect_ctx;
    QOS_VALUE qosValue;
    uint16_t keepAliveInterval;
    MQTT_CLIENT_OPTIONS mqttOptions;
    bool clientConnected;
    bool socketConnected;
    bool logTrace;
    bool rawBytesTrace;
    tickcounter_ms_t timeSincePing;
    uint16_t maxPingRespTime;
} MQTT_CLIENT;

static void on_connection_closed(void* context)
{
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)context;
    if (mqtt_client != NULL)
    {
        mqtt_client->socketConnected = false;
        mqtt_client->clientConnected = false;
        if (mqtt_client->disconnect_cb)
        {
            mqtt_client->disconnect_cb(mqtt_client->disconnect_ctx);
        }
    }
}

static void close_connection(MQTT_CLIENT* mqtt_client)
{
    (void)xio_close(mqtt_client->xioHandle, on_connection_closed, mqtt_client);
    if (mqtt_client->disconnect_cb == NULL)
    {
        size_t counter = 0;
        do
        {
            xio_dowork(mqtt_client->xioHandle);
            counter++;
            ThreadAPI_Sleep(2);
        } while (mqtt_client->clientConnected && counter < MAX_CLOSE_RETRIES);
    }
}

static void set_error_callback(MQTT_CLIENT* mqtt_client, MQTT_CLIENT_EVENT_ERROR error_type)
{
    if (mqtt_client->fnOnErrorCallBack)
    {
        mqtt_client->fnOnErrorCallBack(mqtt_client, error_type, mqtt_client->errorCBCtx);
    }
    close_connection(mqtt_client);
}

static STRING_HANDLE construct_trace_log_handle(MQTT_CLIENT* mqtt_client)
{
    STRING_HANDLE trace_log;
    if (mqtt_client->logTrace)
    {
        trace_log = STRING_new();
    }
    else
    {
        trace_log = NULL;
    }
    return trace_log;
}

static uint16_t byteutil_read_uint16(uint8_t** buffer, size_t len)
{
    uint16_t result = 0;
    if (buffer != NULL && *buffer != NULL && len >= 2)
    {
        result = 256 * (**buffer) + (*(*buffer + 1));
        *buffer += 2; // Move the ptr
    }
    else
    {
        LOG(AZ_LOG_ERROR, LOG_LINE, "byteutil_read_uint16 == NULL or less than 2");
    }
    return result;
}

static char* byteutil_readUTF(uint8_t** buffer, size_t* byteLen)
{
    char* result = NULL;
    if (buffer != NULL)
    {
        // Get the length of the string
        uint16_t len = byteutil_read_uint16(buffer, *byteLen);
        if (len > 0)
        {
            result = (char*)malloc(len + 1);
            if (result != NULL)
            {
                (void)memcpy(result, *buffer, len);
                result[len] = '\0';
                *buffer += len;
                if (byteLen != NULL)
                {
                    *byteLen = len;
                }
            }
        }
    }
    else
    {
        LOG(AZ_LOG_ERROR, LOG_LINE, "readByte buffer == NULL.");
    }
    return result;
}

static uint8_t byteutil_readByte(uint8_t** buffer)
{
    uint8_t result = 0;
    if (buffer != NULL)
    {
        result = **buffer;
        (*buffer)++;
    }
    else
    {
        LOG(AZ_LOG_ERROR, LOG_LINE, "readByte buffer == NULL.");
    }
    return result;
}

static void sendComplete(void* context, IO_SEND_RESULT send_result)
{
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)context;
    if (mqtt_client != NULL)
    {
        if (send_result == IO_SEND_OK)
        {
            if (mqtt_client->packetState == DISCONNECT_TYPE)
            {
                /*Codes_SRS_MQTT_CLIENT_07_032: [If the actionResult parameter is of type MQTT_CLIENT_ON_DISCONNECT the the msgInfo value shall be NULL.]*/
                if (mqtt_client->fnOperationCallback != NULL)
                {
                    mqtt_client->fnOperationCallback(mqtt_client, MQTT_CLIENT_ON_DISCONNECT, NULL, mqtt_client->ctx);
                }
                // close the xio
                close_connection(mqtt_client);
            }
        }
        else if (send_result == IO_SEND_ERROR)
        {
            LOG(AZ_LOG_ERROR, LOG_LINE, "MQTT Send Complete Failure send_result: %d", (int)send_result);
            set_error_callback(mqtt_client, MQTT_CLIENT_COMMUNICATION_ERROR);
        }
    }
    else
    {
        LOG(AZ_LOG_ERROR, LOG_LINE, "MQTT Send Complete Failure with NULL mqtt_client");
    }
}

static const char* retrievePacketType(CONTROL_PACKET_TYPE packet)
{
    switch (packet&CONNECT_PACKET_MASK)
    {
        case CONNECT_TYPE: return "CONNECT";
        case CONNACK_TYPE:  return "CONNACK";
        case PUBLISH_TYPE:  return "PUBLISH";
        case PUBACK_TYPE:  return "PUBACK";
        case PUBREC_TYPE:  return "PUBREC";
        case PUBREL_TYPE:  return "PUBREL";
        case SUBSCRIBE_TYPE:  return "SUBSCRIBE";
        case SUBACK_TYPE:  return "SUBACK";
        case UNSUBSCRIBE_TYPE:  return "UNSUBSCRIBE";
        case UNSUBACK_TYPE:  return "UNSUBACK";
        case PINGREQ_TYPE:  return "PINGREQ";
        case PINGRESP_TYPE:  return "PINGRESP";
        case DISCONNECT_TYPE:  return "DISCONNECT";
        default:
        case PACKET_TYPE_ERROR:
        case UNKNOWN_TYPE:
            return "UNKNOWN";
    }
}

static void getLogTime(char* timeResult, size_t len)
{
    if (timeResult != NULL)
    {
        time_t agent_time = get_time(NULL);
        if (agent_time == (time_t)-1)
        {
            timeResult[0] = '\0';
        }
        else
        {
            struct tm* tmInfo = localtime(&agent_time);
            if (tmInfo == NULL)
            {
                timeResult[0] = '\0';
            }
            else
            {
                if (strftime(timeResult, len, "%H:%M:%S", tmInfo) == 0)
                {
                    timeResult[0] = '\0';
                }
            }
        }
    }
}

static void log_outgoing_trace(MQTT_CLIENT* mqtt_client, STRING_HANDLE trace_log)
{
    if (mqtt_client != NULL && mqtt_client->logTrace && trace_log != NULL)
    {
        char tmBuffer[TIME_MAX_BUFFER];
        getLogTime(tmBuffer, TIME_MAX_BUFFER);
        LOG(AZ_LOG_TRACE, LOG_LINE, "-> %s %s", tmBuffer, STRING_c_str(trace_log));
    }
}

static void logOutgoingRawTrace(MQTT_CLIENT* mqtt_client, const uint8_t* data, size_t length)
{
    if (mqtt_client != NULL && data != NULL && length > 0 && mqtt_client->rawBytesTrace)
    {
        char tmBuffer[TIME_MAX_BUFFER];
        getLogTime(tmBuffer, TIME_MAX_BUFFER);

        LOG(AZ_LOG_TRACE, 0, "-> %s %s: ", tmBuffer, retrievePacketType((unsigned char)data[0]));
        size_t index = 0;
        for (index = 0; index < length; index++)
        {
            LOG(AZ_LOG_TRACE, 0, "0x%02x ", data[index]);
        }
        LOG(AZ_LOG_TRACE, LOG_LINE, "");
    }
}

static void log_incoming_trace(MQTT_CLIENT* mqtt_client, STRING_HANDLE trace_log)
{
    if (mqtt_client != NULL && mqtt_client->logTrace && trace_log != NULL)
    {
        char tmBuffer[TIME_MAX_BUFFER];
        getLogTime(tmBuffer, TIME_MAX_BUFFER);
        LOG(AZ_LOG_TRACE, LOG_LINE, "<- %s %s", tmBuffer, STRING_c_str(trace_log) );
    }
}

static void logIncomingRawTrace(MQTT_CLIENT* mqtt_client, CONTROL_PACKET_TYPE packet, uint8_t flags, const uint8_t* data, size_t length)
{
#ifdef NO_LOGGING
    UNUSED(flags);
#endif
    if (mqtt_client != NULL && mqtt_client->rawBytesTrace)
    {
        if (data != NULL && length > 0)
        {
            char tmBuffer[TIME_MAX_BUFFER];
            getLogTime(tmBuffer, TIME_MAX_BUFFER);

            LOG(AZ_LOG_TRACE, 0, "<- %s %s: 0x%02x 0x%02x ", tmBuffer, retrievePacketType((CONTROL_PACKET_TYPE)packet), (unsigned char)(packet | flags), length);
            size_t index = 0;
            for (index = 0; index < length; index++)
            {
                LOG(AZ_LOG_TRACE, 0, "0x%02x ", data[index]);
            }
            LOG(AZ_LOG_TRACE, LOG_LINE, "");
        }
        else if (packet == PINGRESP_TYPE)
        {
            char tmBuffer[TIME_MAX_BUFFER];
            getLogTime(tmBuffer, TIME_MAX_BUFFER);
            LOG(AZ_LOG_TRACE, LOG_LINE, "<- %s %s: 0x%02x 0x%02x ", tmBuffer, retrievePacketType((CONTROL_PACKET_TYPE)packet), (unsigned char)(packet | flags), length);
        }
    }
}

static int sendPacketItem(MQTT_CLIENT* mqtt_client, const unsigned char* data, size_t length)
{
    int result;

    if (tickcounter_get_current_ms(mqtt_client->packetTickCntr, &mqtt_client->packetSendTimeMs) != 0)
    {
        LOG(AZ_LOG_ERROR, LOG_LINE, "Failure getting current ms tickcounter");
        result = __FAILURE__;
    }
    else
    {
        result = xio_send(mqtt_client->xioHandle, (const void*)data, length, sendComplete, mqtt_client);
        if (result != 0)
        {
            LOG(AZ_LOG_ERROR, LOG_LINE, "%d: Failure sending control packet data", result);
            result = __FAILURE__;
        }
        else
        {
            logOutgoingRawTrace(mqtt_client, (const uint8_t*)data, length);
        }
    }
    return result;
}

static void onOpenComplete(void* context, IO_OPEN_RESULT open_result)
{
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)context;
    if (mqtt_client != NULL)
    {
        if (open_result == IO_OPEN_OK && !mqtt_client->socketConnected)
        {
            mqtt_client->packetState = CONNECT_TYPE;
            mqtt_client->socketConnected = true;

            STRING_HANDLE trace_log = construct_trace_log_handle(mqtt_client);

            // Send the Connect packet
            BUFFER_HANDLE connPacket = mqtt_codec_connect(&mqtt_client->mqttOptions, trace_log);
            if (connPacket == NULL)
            {
                LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_codec_connect failed");
            }
            else
            {
                size_t size = BUFFER_length(connPacket);
                /*Codes_SRS_MQTT_CLIENT_07_009: [On success mqtt_client_connect shall send the MQTT CONNECT to the endpoint.]*/
                if (sendPacketItem(mqtt_client, BUFFER_u_char(connPacket), size) != 0)
                {
                    LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_codec_connect failed");
                }
                else
                {
                    log_outgoing_trace(mqtt_client, trace_log);
                }
                BUFFER_delete(connPacket);
            }
            if (trace_log != NULL)
            {
                STRING_delete(trace_log);
            }
        }
        else
        {
            if (mqtt_client->socketConnected == false && mqtt_client->fnOnErrorCallBack)
            {
                mqtt_client->fnOnErrorCallBack(mqtt_client, MQTT_CLIENT_CONNECTION_ERROR, mqtt_client->errorCBCtx);
            }
            close_connection(mqtt_client);
        }
    }
    else
    {
        LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_client is NULL");
    }
}

static void onBytesReceived(void* context, const unsigned char* buffer, size_t size)
{
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)context;
    if (mqtt_client != NULL)
    {
        if (mqtt_codec_bytesReceived(mqtt_client->codec_handle, buffer, size) != 0)
        {
            set_error_callback(mqtt_client, MQTT_CLIENT_PARSE_ERROR);
        }
    }
    else
    {
        LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_client is NULL");
    }
}

static void onIoError(void* context)
{
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)context;
    if (mqtt_client != NULL && mqtt_client->fnOperationCallback)
    {
        /*Codes_SRS_MQTT_CLIENT_07_032: [If the actionResult parameter is of type MQTT_CLIENT_ON_DISCONNECT the the msgInfo value shall be NULL.]*/
        /* Codes_SRS_MQTT_CLIENT_07_036: [ If an error is encountered by the ioHandle the mqtt_client shall call xio_close. ] */
        set_error_callback(mqtt_client, MQTT_CLIENT_CONNECTION_ERROR);
    }
    else
    {
        LOG(AZ_LOG_ERROR, LOG_LINE, "Error invalid parameter: mqtt_client: %p", mqtt_client);
    }
}

static void clear_mqtt_options(MQTT_CLIENT* mqtt_client)
{
    if (mqtt_client->mqttOptions.clientId != NULL)
    {
        free(mqtt_client->mqttOptions.clientId);
        mqtt_client->mqttOptions.clientId = NULL;
    }

    if (mqtt_client->mqttOptions.willTopic != NULL)
    {
        free(mqtt_client->mqttOptions.willTopic);
        mqtt_client->mqttOptions.willTopic = NULL;
    }

    if (mqtt_client->mqttOptions.willMessage != NULL)
    {
        free(mqtt_client->mqttOptions.willMessage);
        mqtt_client->mqttOptions.willMessage = NULL;
    }

    if (mqtt_client->mqttOptions.username != NULL)
    {
        free(mqtt_client->mqttOptions.username);
        mqtt_client->mqttOptions.username = NULL;
    }

    if (mqtt_client->mqttOptions.password != NULL)
    {
        free(mqtt_client->mqttOptions.password);
        mqtt_client->mqttOptions.password = NULL;
    }
}

static int cloneMqttOptions(MQTT_CLIENT* mqtt_client, const MQTT_CLIENT_OPTIONS* mqttOptions)
{
    int result = 0;

    if (mqttOptions->clientId != NULL)
    {
        char* clientId;

        if (mallocAndStrcpy_s(&clientId, mqttOptions->clientId) != 0)
        {
            result = __FAILURE__;
            LOG(AZ_LOG_ERROR, LOG_LINE, "mallocAndStrcpy_s clientId");
        }
        else
        {
            if (mqtt_client->mqttOptions.clientId != NULL)
            {
                free(mqtt_client->mqttOptions.clientId);
            }

            mqtt_client->mqttOptions.clientId = clientId;
        }
    }
    if (result == 0 && mqttOptions->willTopic != NULL)
    {
        char* willTopic;

        if (mallocAndStrcpy_s(&willTopic, mqttOptions->willTopic) != 0)
        {
            result = __FAILURE__;
            LOG(AZ_LOG_ERROR, LOG_LINE, "mallocAndStrcpy_s willTopic");
        }
        else
        {
            if (mqtt_client->mqttOptions.willTopic != NULL)
            {
                free(mqtt_client->mqttOptions.willTopic);
            }

            mqtt_client->mqttOptions.willTopic = willTopic;
        }
    }
    if (result == 0 && mqttOptions->willMessage != NULL)
    {
        char* willMessage;

        if (mallocAndStrcpy_s(&willMessage, mqttOptions->willMessage) != 0)
        {
            LOG(AZ_LOG_ERROR, LOG_LINE, "mallocAndStrcpy_s willMessage");
            result = __FAILURE__;
        }
        else
        {
            if (mqtt_client->mqttOptions.willMessage != NULL)
            {
                free(mqtt_client->mqttOptions.willMessage);
            }

            mqtt_client->mqttOptions.willMessage = willMessage;
        }
    }
    if (result == 0 && mqttOptions->username != NULL)
    {
        char* username;

        if (mallocAndStrcpy_s(&username, mqttOptions->username) != 0)
        {
            LOG(AZ_LOG_ERROR, LOG_LINE, "mallocAndStrcpy_s username");
            result = __FAILURE__;
        }
        else
        {
            if (mqtt_client->mqttOptions.username != NULL)
            {
                free(mqtt_client->mqttOptions.username);
            }

            mqtt_client->mqttOptions.username = username;
        }
    }
    if (result == 0 && mqttOptions->password != NULL)
    {
        char* password;

        if (mallocAndStrcpy_s(&password, mqttOptions->password) != 0)
        {
            LOG(AZ_LOG_ERROR, LOG_LINE, "mallocAndStrcpy_s password");
            result = __FAILURE__;
        }
        else
        {
            if (mqtt_client->mqttOptions.password != NULL)
            {
                free(mqtt_client->mqttOptions.password);
            }

            mqtt_client->mqttOptions.password = password;
        }
    }
    if (result == 0)
    {
        mqtt_client->mqttOptions.keepAliveInterval = mqttOptions->keepAliveInterval;
        mqtt_client->mqttOptions.messageRetain = mqttOptions->messageRetain;
        mqtt_client->mqttOptions.useCleanSession = mqttOptions->useCleanSession;
        mqtt_client->mqttOptions.qualityOfServiceValue = mqttOptions->qualityOfServiceValue;
    }
    else
    {
        clear_mqtt_options(mqtt_client);
    }
    return result;
}

static void recvCompleteCallback(void* context, CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData)
{
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)context;
    if ((mqtt_client != NULL && headerData != NULL) || packet == PINGRESP_TYPE)
    {
        size_t len = BUFFER_length(headerData);
        uint8_t* iterator = BUFFER_u_char(headerData);

        logIncomingRawTrace(mqtt_client, packet, (uint8_t)flags, iterator, len);

        if ((iterator != NULL && len > 0) || packet == PINGRESP_TYPE)
        {
            switch (packet)
            {
                case CONNACK_TYPE:
                {
                    if (mqtt_client->fnOperationCallback != NULL)
                    {
                        STRING_HANDLE trace_log = NULL;

                        /*Codes_SRS_MQTT_CLIENT_07_028: [If the actionResult parameter is of type CONNECT_ACK then the msgInfo value shall be a CONNECT_ACK structure.]*/
                        CONNECT_ACK connack = { 0 };
                        connack.isSessionPresent = (byteutil_readByte(&iterator) == 0x1) ? true : false;
                        connack.returnCode = byteutil_readByte(&iterator);

                        if (mqtt_client->logTrace)
                        {
                            trace_log = STRING_construct_sprintf("CONNACK | SESSION_PRESENT: %s | RETURN_CODE: 0x%x", connack.isSessionPresent ? TRUE_CONST : FALSE_CONST, connack.returnCode);
                            log_incoming_trace(mqtt_client, trace_log);
                            STRING_delete(trace_log);
                        }
                        mqtt_client->fnOperationCallback(mqtt_client, MQTT_CLIENT_ON_CONNACK, (void*)&connack, mqtt_client->ctx);

                        if (connack.returnCode == CONNECTION_ACCEPTED)
                        {
                            mqtt_client->clientConnected = true;
                        }
                    }
                    else
                    {
                        LOG(AZ_LOG_ERROR, LOG_LINE, "fnOperationCallback NULL");
                    }
                    break;
                }
                case PUBLISH_TYPE:
                {
                    if (mqtt_client->fnMessageRecv != NULL)
                    {
                        STRING_HANDLE trace_log = NULL;

                        bool isDuplicateMsg = (flags & DUPLICATE_FLAG_MASK) ? true : false;
                        bool isRetainMsg = (flags & RETAIN_FLAG_MASK) ? true : false;
                        QOS_VALUE qosValue = (flags == 0) ? DELIVER_AT_MOST_ONCE : (flags & QOS_LEAST_ONCE_FLAG_MASK) ? DELIVER_AT_LEAST_ONCE : DELIVER_EXACTLY_ONCE;

                        if (mqtt_client->logTrace)
                        {
                            trace_log = STRING_construct_sprintf("PUBLISH | IS_DUP: %s | RETAIN: %d | QOS: %s", isDuplicateMsg ? TRUE_CONST : FALSE_CONST, 
                                isRetainMsg ? 1 : 0, ENUM_TO_STRING(QOS_VALUE, qosValue) );
                        }

                        uint8_t* initialPos = iterator;
                        size_t length = len - (iterator - initialPos);
                        char* topicName = byteutil_readUTF(&iterator, &length);
                        if (topicName == NULL)
                        {
                            LOG(AZ_LOG_ERROR, LOG_LINE, "Publish MSG: failure reading topic name");
                            set_error_callback(mqtt_client, MQTT_CLIENT_PARSE_ERROR);
                            if (trace_log != NULL)
                            {
                                STRING_delete(trace_log);
                            }
                        }
                        else
                        {
                            if (mqtt_client->logTrace)
                            {
                                STRING_sprintf(trace_log, " | TOPIC_NAME: %s", topicName);
                            }
                            uint16_t packetId = 0;
                            length = len - (iterator - initialPos);
                            if (qosValue != DELIVER_AT_MOST_ONCE)
                            {
                                packetId = byteutil_read_uint16(&iterator, length);
                                if (mqtt_client->logTrace)
                                {
                                    STRING_sprintf(trace_log, " | PACKET_ID: %"PRIu16, packetId);
                                }
                            }
                            length = len - (iterator - initialPos);

                            MQTT_MESSAGE_HANDLE msgHandle = mqttmessage_create(packetId, topicName, qosValue, iterator, length);
                            if (msgHandle == NULL)
                            {
                                LOG(AZ_LOG_ERROR, LOG_LINE, "failure in mqttmessage_create");
                                set_error_callback(mqtt_client, MQTT_CLIENT_MEMORY_ERROR);
                                if (trace_log != NULL) {
                                    STRING_delete(trace_log);
                                }
                            }
                            else
                            {
                                if (mqttmessage_setIsDuplicateMsg(msgHandle, isDuplicateMsg) != 0 ||
                                    mqttmessage_setIsRetained(msgHandle, isRetainMsg) != 0)
                                {
                                    LOG(AZ_LOG_ERROR, LOG_LINE, "failure setting mqtt message property");
                                    set_error_callback(mqtt_client, MQTT_CLIENT_MEMORY_ERROR);
                                    if (trace_log != NULL) {
                                        STRING_delete(trace_log);
                                    }
                                }
                                else
                                {
                                    if (mqtt_client->logTrace)
                                    {
                                        STRING_sprintf(trace_log, " | PAYLOAD_LEN: %lu", length);
                                        log_incoming_trace(mqtt_client, trace_log);
                                        STRING_delete(trace_log);
                                    }

                                    mqtt_client->fnMessageRecv(msgHandle, mqtt_client->ctx);

                                    BUFFER_HANDLE pubRel = NULL;
                                    if (qosValue == DELIVER_EXACTLY_ONCE)
                                    {
                                        pubRel = mqtt_codec_publishReceived(packetId);
                                        if (pubRel == NULL)
                                        {
                                            LOG(AZ_LOG_ERROR, LOG_LINE, "Failed to allocate publish receive message.");
                                            set_error_callback(mqtt_client, MQTT_CLIENT_MEMORY_ERROR);
                                        }
                                    }
                                    else if (qosValue == DELIVER_AT_LEAST_ONCE)
                                    {
                                        pubRel = mqtt_codec_publishAck(packetId);
                                        if (pubRel == NULL)
                                        {
                                            LOG(AZ_LOG_ERROR, LOG_LINE, "Failed to allocate publish ack message.");
                                            set_error_callback(mqtt_client, MQTT_CLIENT_MEMORY_ERROR);
                                        }
                                    }
                                    if (pubRel != NULL)
                                    {
                                        size_t size = BUFFER_length(pubRel);
                                        (void)sendPacketItem(mqtt_client, BUFFER_u_char(pubRel), size);
                                        BUFFER_delete(pubRel);
                                    }
                                }
                                mqttmessage_destroy(msgHandle);
                            }
                            free(topicName);
                        }
                    }
                    break;
                }
                case PUBACK_TYPE:
                case PUBREC_TYPE:
                case PUBREL_TYPE:
                case PUBCOMP_TYPE:
                {
                    if (mqtt_client->fnOperationCallback)
                    {
                        STRING_HANDLE trace_log = NULL;

                        /*Codes_SRS_MQTT_CLIENT_07_029: [If the actionResult parameter are of types PUBACK_TYPE, PUBREC_TYPE, PUBREL_TYPE or PUBCOMP_TYPE then the msgInfo value shall be a PUBLISH_ACK structure.]*/
                        MQTT_CLIENT_EVENT_RESULT action = (packet == PUBACK_TYPE) ? MQTT_CLIENT_ON_PUBLISH_ACK :
                            (packet == PUBREC_TYPE) ? MQTT_CLIENT_ON_PUBLISH_RECV :
                            (packet == PUBREL_TYPE) ? MQTT_CLIENT_ON_PUBLISH_REL : MQTT_CLIENT_ON_PUBLISH_COMP;

                        PUBLISH_ACK publish_ack = { 0 };
                        publish_ack.packetId = byteutil_read_uint16(&iterator, len);

                        if (mqtt_client->logTrace)
                        {
                            trace_log = STRING_construct_sprintf("%s | PACKET_ID: %"PRIu16, packet == PUBACK_TYPE ? "PUBACK" : (packet == PUBREC_TYPE) ? "PUBREC" : (packet == PUBREL_TYPE) ? "PUBREL" : "PUBCOMP",
                                publish_ack.packetId);

                            log_incoming_trace(mqtt_client, trace_log);
                            STRING_delete(trace_log);
                        }

                        BUFFER_HANDLE pubRel = NULL;
                        mqtt_client->fnOperationCallback(mqtt_client, action, (void*)&publish_ack, mqtt_client->ctx);
                        if (packet == PUBREC_TYPE)
                        {
                            pubRel = mqtt_codec_publishRelease(publish_ack.packetId);
                            if (pubRel == NULL)
                            {
                                LOG(AZ_LOG_ERROR, LOG_LINE, "Failed to allocate publish release message.");
                                set_error_callback(mqtt_client, MQTT_CLIENT_MEMORY_ERROR);
                            }
                        }
                        else if (packet == PUBREL_TYPE)
                        {
                            pubRel = mqtt_codec_publishComplete(publish_ack.packetId);
                            if (pubRel == NULL)
                            {
                                LOG(AZ_LOG_ERROR, LOG_LINE, "Failed to allocate publish complete message.");
                                set_error_callback(mqtt_client, MQTT_CLIENT_MEMORY_ERROR);
                            }
                        }
                        if (pubRel != NULL)
                        {
                            size_t size = BUFFER_length(pubRel);
                            (void)sendPacketItem(mqtt_client, BUFFER_u_char(pubRel), size);
                            BUFFER_delete(pubRel);
                        }
                    }
                    break;
                }
                case SUBACK_TYPE:
                {
                    if (mqtt_client->fnOperationCallback)
                    {
                        STRING_HANDLE trace_log = NULL;

                        /*Codes_SRS_MQTT_CLIENT_07_030: [If the actionResult parameter is of type SUBACK_TYPE then the msgInfo value shall be a SUBSCRIBE_ACK structure.]*/
                        SUBSCRIBE_ACK suback = { 0 };

                        size_t remainLen = len;
                        suback.packetId = byteutil_read_uint16(&iterator, len);
                        remainLen -= 2;

                        if (mqtt_client->logTrace)
                        {
                            trace_log = STRING_construct_sprintf("SUBACK | PACKET_ID: %"PRIu16, suback.packetId);
                        }

                        // Allocate the remaining len
                        suback.qosReturn = (QOS_VALUE*)malloc(sizeof(QOS_VALUE)*remainLen);
                        if (suback.qosReturn != NULL)
                        {
                            while (remainLen > 0)
                            {
                                suback.qosReturn[suback.qosCount++] = byteutil_readByte(&iterator);
                                remainLen--;
                                if (mqtt_client->logTrace)
                                {
                                    STRING_sprintf(trace_log, " | RETURN_CODE: %"PRIu16, suback.qosReturn[suback.qosCount-1]);
                                }
                            }

                            if (mqtt_client->logTrace)
                            {
                                log_incoming_trace(mqtt_client, trace_log);
                                STRING_delete(trace_log);
                            }
                            mqtt_client->fnOperationCallback(mqtt_client, MQTT_CLIENT_ON_SUBSCRIBE_ACK, (void*)&suback, mqtt_client->ctx);
                            free(suback.qosReturn);
                        }
                        else
                        {
                            LOG(AZ_LOG_ERROR, LOG_LINE, "allocation of quality of service value failed.");
                            set_error_callback(mqtt_client, MQTT_CLIENT_MEMORY_ERROR);
                        }
                    }
                    break;
                }
                case UNSUBACK_TYPE:
                {
                    if (mqtt_client->fnOperationCallback)
                    {
                        STRING_HANDLE trace_log = NULL;

                        /*Codes_SRS_MQTT_CLIENT_07_031: [If the actionResult parameter is of type UNSUBACK_TYPE then the msgInfo value shall be a UNSUBSCRIBE_ACK structure.]*/
                        UNSUBSCRIBE_ACK unsuback = { 0 };
                        unsuback.packetId = byteutil_read_uint16(&iterator, len);

                        if (mqtt_client->logTrace)
                        {
                            trace_log = STRING_construct_sprintf("UNSUBACK | PACKET_ID: %"PRIu16, unsuback.packetId);
                            log_incoming_trace(mqtt_client, trace_log);
                            STRING_delete(trace_log);
                        }
                        mqtt_client->fnOperationCallback(mqtt_client, MQTT_CLIENT_ON_UNSUBSCRIBE_ACK, (void*)&unsuback, mqtt_client->ctx);
                    }
                    break;
                }
                case PINGRESP_TYPE:
                    mqtt_client->timeSincePing = 0;
                    if (mqtt_client->logTrace)
                    {
                        STRING_HANDLE trace_log = STRING_construct_sprintf("PINGRESP");
                        log_incoming_trace(mqtt_client, trace_log);
                        STRING_delete(trace_log);
                    }
                    // Forward ping response to operation callback 
                    if (mqtt_client->fnOperationCallback)
                    {
                        mqtt_client->fnOperationCallback(mqtt_client, MQTT_CLIENT_ON_PING_RESPONSE, NULL, mqtt_client->ctx);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

MQTT_CLIENT_HANDLE mqtt_client_init(ON_MQTT_MESSAGE_RECV_CALLBACK msgRecv, ON_MQTT_OPERATION_CALLBACK opCallback, void* opCallbackCtx, ON_MQTT_ERROR_CALLBACK onErrorCallBack, void* errorCBCtx)
{
    MQTT_CLIENT* result;
    /*Codes_SRS_MQTT_CLIENT_07_001: [If the parameters ON_MQTT_MESSAGE_RECV_CALLBACK is NULL then mqttclient_init shall return NULL.]*/
    if (msgRecv == NULL)
    {
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(MQTT_CLIENT));
        if (result == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_002: [If any failure is encountered then mqttclient_init shall return NULL.]*/
            LOG(AZ_LOG_ERROR, LOG_LINE, "mqtt_client_init failure: Allocation Failure");
        }
        else
        {
            memset(result, 0, sizeof(MQTT_CLIENT));
            /*Codes_SRS_MQTT_CLIENT_07_003: [mqttclient_init shall allocate MQTTCLIENT_DATA_INSTANCE and return the MQTTCLIENT_HANDLE on success.]*/
            result->packetState = UNKNOWN_TYPE;
            result->fnOperationCallback = opCallback;
            result->ctx = opCallbackCtx;
            result->fnMessageRecv = msgRecv;
            result->fnOnErrorCallBack = onErrorCallBack;
            result->errorCBCtx = errorCBCtx;
            result->qosValue = DELIVER_AT_MOST_ONCE;
            result->packetTickCntr = tickcounter_create();
            result->maxPingRespTime = DEFAULT_MAX_PING_RESPONSE_TIME;
            if (result->packetTickCntr == NULL)
            {
                /*Codes_SRS_MQTT_CLIENT_07_002: [If any failure is encountered then mqttclient_init shall return NULL.]*/
                LOG(AZ_LOG_ERROR, LOG_LINE, "mqtt_client_init failure: tickcounter_create failure");
                free(result);
                result = NULL;
            }
            else
            {
                result->codec_handle = mqtt_codec_create(recvCompleteCallback, result);
                if (result->codec_handle == NULL)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_002: [If any failure is encountered then mqttclient_init shall return NULL.]*/
                    LOG(AZ_LOG_ERROR, LOG_LINE, "mqtt_client_init failure: mqtt_codec_create failure");
                    tickcounter_destroy(result->packetTickCntr);
                    free(result);
                    result = NULL;
                }
            }
        }
    }
    return result;
}

void mqtt_client_deinit(MQTT_CLIENT_HANDLE handle)
{
    /*Codes_SRS_MQTT_CLIENT_07_004: [If the parameter handle is NULL then function mqtt_client_deinit shall do nothing.]*/
    if (handle != NULL)
    {
        /*Codes_SRS_MQTT_CLIENT_07_005: [mqtt_client_deinit shall deallocate all memory allocated in this unit.]*/
        MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)handle;
        tickcounter_destroy(mqtt_client->packetTickCntr);
        mqtt_codec_destroy(mqtt_client->codec_handle);
        clear_mqtt_options(mqtt_client);
        free(mqtt_client);
    }
}

int mqtt_client_connect(MQTT_CLIENT_HANDLE handle, XIO_HANDLE xioHandle, MQTT_CLIENT_OPTIONS* mqttOptions)
{
    int result;
    /*SRS_MQTT_CLIENT_07_006: [If any of the parameters handle, ioHandle, or mqttOptions are NULL then mqtt_client_connect shall return a non-zero value.]*/
    if (handle == NULL || mqttOptions == NULL)
    {
        LOG(AZ_LOG_ERROR, LOG_LINE, "mqtt_client_connect: NULL argument (handle = %p, mqttOptions = %p)", handle, mqttOptions);
        result = __FAILURE__;
    }
    else
    {
        MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)handle;
        if (xioHandle == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
            LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqttcodec_connect failed");
            result = __FAILURE__;
        }
        else
        {
            mqtt_client->xioHandle = xioHandle;
            mqtt_client->packetState = UNKNOWN_TYPE;
            mqtt_client->qosValue = mqttOptions->qualityOfServiceValue;
            mqtt_client->keepAliveInterval = mqttOptions->keepAliveInterval;
            mqtt_client->maxPingRespTime = (DEFAULT_MAX_PING_RESPONSE_TIME < mqttOptions->keepAliveInterval/2) ? DEFAULT_MAX_PING_RESPONSE_TIME : mqttOptions->keepAliveInterval/2;
            if (cloneMqttOptions(mqtt_client, mqttOptions) != 0)
            {
                LOG(AZ_LOG_ERROR, LOG_LINE, "Error: Clone Mqtt Options failed");
                result = __FAILURE__;
            }
            /*Codes_SRS_MQTT_CLIENT_07_008: [mqtt_client_connect shall open the XIO_HANDLE by calling into the xio_open interface.]*/
            else if (xio_open(xioHandle, onOpenComplete, mqtt_client, onBytesReceived, mqtt_client, onIoError, mqtt_client) != 0)
            {
                /*Codes_SRS_MQTT_CLIENT_07_007: [If any failure is encountered then mqtt_client_connect shall return a non-zero value.]*/
                LOG(AZ_LOG_ERROR, LOG_LINE, "Error: io_open failed");
                result = __FAILURE__;
                // Remove cloned options
                clear_mqtt_options(mqtt_client);
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

int mqtt_client_publish(MQTT_CLIENT_HANDLE handle, MQTT_MESSAGE_HANDLE msgHandle)
{
    int result;
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)handle;
    if (mqtt_client == NULL || msgHandle == NULL)
    {
        /*Codes_SRS_MQTT_CLIENT_07_019: [If one of the parameters handle or msgHandle is NULL then mqtt_client_publish shall return a non-zero value.]*/
        LogError("Invalid parameter specified mqtt_client: %p, msgHandle: %p", mqtt_client, msgHandle);
        result = __FAILURE__;
    }
    else
    {
        /*Codes_SRS_MQTT_CLIENT_07_021: [mqtt_client_publish shall get the message information from the MQTT_MESSAGE_HANDLE.]*/
        const APP_PAYLOAD* payload = mqttmessage_getApplicationMsg(msgHandle);
        if (payload == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_020: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.]*/
            LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqttmessage_getApplicationMsg failed");
            result = __FAILURE__;
        }
        else
        {
            STRING_HANDLE trace_log = construct_trace_log_handle(mqtt_client);

            QOS_VALUE qos = mqttmessage_getQosType(msgHandle);
            bool isDuplicate = mqttmessage_getIsDuplicateMsg(msgHandle);
            bool isRetained = mqttmessage_getIsRetained(msgHandle);
            uint16_t packetId = mqttmessage_getPacketId(msgHandle);
            const char* topicName = mqttmessage_getTopicName(msgHandle);
            BUFFER_HANDLE publishPacket = mqtt_codec_publish(qos, isDuplicate, isRetained, packetId, topicName, payload->message, payload->length, trace_log);
            if (publishPacket == NULL)
            {
                /*Codes_SRS_MQTT_CLIENT_07_020: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.]*/
                LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_codec_publish failed");
                result = __FAILURE__;
            }
            else
            {
                mqtt_client->packetState = PUBLISH_TYPE;

                /*Codes_SRS_MQTT_CLIENT_07_022: [On success mqtt_client_publish shall send the MQTT SUBCRIBE packet to the endpoint.]*/
                size_t size = BUFFER_length(publishPacket);
                if (sendPacketItem(mqtt_client, BUFFER_u_char(publishPacket), size) != 0)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_020: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.]*/
                    LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_client_publish send failed");
                    result = __FAILURE__;
                }
                else
                {
                    log_outgoing_trace(mqtt_client, trace_log);
                    result = 0;
                }
                BUFFER_delete(publishPacket);
            }
            if (trace_log != NULL)
            {
                STRING_delete(trace_log);
            }
        }
    }
    return result;
}

int mqtt_client_subscribe(MQTT_CLIENT_HANDLE handle, uint16_t packetId, SUBSCRIBE_PAYLOAD* subscribeList, size_t count)
{
    int result;
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)handle;
    if (mqtt_client == NULL || subscribeList == NULL || count == 0 || packetId == 0)
    {
        /*Codes_SRS_MQTT_CLIENT_07_013: [If any of the parameters handle, subscribeList is NULL or count is 0 then mqtt_client_subscribe shall return a non-zero value.]*/
        LogError("Invalid parameter specified mqtt_client: %p, subscribeList: %p, count: %d, packetId: %d", mqtt_client, subscribeList, count, packetId);
        result = __FAILURE__;
    }
    else
    {
        STRING_HANDLE trace_log = construct_trace_log_handle(mqtt_client);

        BUFFER_HANDLE subPacket = mqtt_codec_subscribe(packetId, subscribeList, count, trace_log);
        if (subPacket == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_014: [If any failure is encountered then mqtt_client_subscribe shall return a non-zero value.]*/
            LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_codec_subscribe failed");
            result = __FAILURE__;
        }
        else
        {
            mqtt_client->packetState = SUBSCRIBE_TYPE;

            size_t size = BUFFER_length(subPacket);
            /*Codes_SRS_MQTT_CLIENT_07_015: [On success mqtt_client_subscribe shall send the MQTT SUBCRIBE packet to the endpoint.]*/
            if (sendPacketItem(mqtt_client, BUFFER_u_char(subPacket), size) != 0)
            {
                /*Codes_SRS_MQTT_CLIENT_07_014: [If any failure is encountered then mqtt_client_subscribe shall return a non-zero value.]*/
                LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_client_subscribe send failed");
                result = __FAILURE__;
            }
            else
            {
                log_outgoing_trace(mqtt_client, trace_log);
                result = 0;
            }
            BUFFER_delete(subPacket);
        }
        if (trace_log != NULL)
        {
            STRING_delete(trace_log);
        }
    }
    return result;
}

int mqtt_client_unsubscribe(MQTT_CLIENT_HANDLE handle, uint16_t packetId, const char** unsubscribeList, size_t count)
{
    int result;
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)handle;
    if (mqtt_client == NULL || unsubscribeList == NULL || count == 0 || packetId == 0)
    {
        /*Codes_SRS_MQTT_CLIENT_07_016: [If any of the parameters handle, unsubscribeList is NULL or count is 0 then mqtt_client_unsubscribe shall return a non-zero value.]*/
        LogError("Invalid parameter specified mqtt_client: %p, unsubscribeList: %p, count: %d, packetId: %d", mqtt_client, unsubscribeList, count, packetId);
        result = __FAILURE__;
    }
    else
    {
        STRING_HANDLE trace_log = construct_trace_log_handle(mqtt_client);

        BUFFER_HANDLE unsubPacket = mqtt_codec_unsubscribe(packetId, unsubscribeList, count, trace_log);
        if (unsubPacket == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_017: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.]*/
            LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_codec_unsubscribe failed");
            result = __FAILURE__;
        }
        else
        {
            mqtt_client->packetState = UNSUBSCRIBE_TYPE;

            size_t size = BUFFER_length(unsubPacket);
            /*Codes_SRS_MQTT_CLIENT_07_018: [On success mqtt_client_unsubscribe shall send the MQTT SUBCRIBE packet to the endpoint.]*/
            if (sendPacketItem(mqtt_client, BUFFER_u_char(unsubPacket), size) != 0)
            {
                /*Codes_SRS_MQTT_CLIENT_07_017: [If any failure is encountered then mqtt_client_unsubscribe shall return a non-zero value.].]*/
                LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_client_unsubscribe send failed");
                result = __FAILURE__;
            }
            else
            {
                log_outgoing_trace(mqtt_client, trace_log);
                result = 0;
            }
            BUFFER_delete(unsubPacket);
        }
        if (trace_log != NULL)
        {
            STRING_delete(trace_log);
        }
    }
    return result;
}

int mqtt_client_disconnect(MQTT_CLIENT_HANDLE handle, ON_MQTT_DISCONNECTED_CALLBACK callback, void* ctx)
{
    int result = 0;
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)handle;
    if (mqtt_client == NULL)
    {
        /*Codes_SRS_MQTT_CLIENT_07_010: [If the parameters handle is NULL then mqtt_client_disconnect shall return a non-zero value.]*/
        result = __FAILURE__;
    }
    else if (mqtt_client->xioHandle != NULL)
    {
        BUFFER_HANDLE disconnectPacket = mqtt_codec_disconnect();
        if (disconnectPacket == NULL)
        {
            /*Codes_SRS_MQTT_CLIENT_07_011: [If any failure is encountered then mqtt_client_disconnect shall return a non-zero value.]*/
            LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_client_disconnect failed");
            mqtt_client->packetState = PACKET_TYPE_ERROR;
            result = __FAILURE__;
        }
        else
        {
            /* Codes_SRS_MQTT_CLIENT_07_037: [ if callback is not NULL callback shall be called once the mqtt connection has been disconnected ] */
            mqtt_client->disconnect_cb = callback;
            mqtt_client->disconnect_ctx = ctx;
            mqtt_client->packetState = DISCONNECT_TYPE;

            size_t size = BUFFER_length(disconnectPacket);
            /*Codes_SRS_MQTT_CLIENT_07_012: [On success mqtt_client_disconnect shall send the MQTT DISCONNECT packet to the endpoint.]*/
            if (sendPacketItem(mqtt_client, BUFFER_u_char(disconnectPacket), size) != 0)
            {
                /*Codes_SRS_MQTT_CLIENT_07_011: [If any failure is encountered then mqtt_client_disconnect shall return a non-zero value.]*/
                LOG(AZ_LOG_ERROR, LOG_LINE, "Error: mqtt_client_disconnect send failed");
                result = __FAILURE__;
            }
            else
            {
                if (mqtt_client->logTrace)
                {
                    STRING_HANDLE trace_log = STRING_construct("DISCONNECT");
                    log_outgoing_trace(mqtt_client, trace_log);
                    STRING_delete(trace_log);
                }
                result = 0;
            }
            BUFFER_delete(disconnectPacket);
            clear_mqtt_options(mqtt_client);
            mqtt_client->xioHandle = NULL;
        }
    }
    return result;
}

void mqtt_client_dowork(MQTT_CLIENT_HANDLE handle)
{
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)handle;
    /*Codes_SRS_MQTT_CLIENT_18_001: [If the client is disconnected, mqtt_client_dowork shall do nothing.]*/
    /*Codes_SRS_MQTT_CLIENT_07_023: [If the parameter handle is NULL then mqtt_client_dowork shall do nothing.]*/
    if (mqtt_client != NULL && mqtt_client->xioHandle != NULL)
    {
        /*Codes_SRS_MQTT_CLIENT_07_024: [mqtt_client_dowork shall call the xio_dowork function to complete operations.]*/
        xio_dowork(mqtt_client->xioHandle);

        /*Codes_SRS_MQTT_CLIENT_07_025: [mqtt_client_dowork shall retrieve the the last packet send value and ...]*/
        if (mqtt_client->socketConnected && mqtt_client->clientConnected && mqtt_client->keepAliveInterval > 0)
        {
            tickcounter_ms_t current_ms;
            if (tickcounter_get_current_ms(mqtt_client->packetTickCntr, &current_ms) != 0)
            {
                LOG(AZ_LOG_ERROR, LOG_LINE, "Error: tickcounter_get_current_ms failed");
            }
            else
            {
                /* Codes_SRS_MQTT_CLIENT_07_035: [If the timeSincePing has expired past the maxPingRespTime then mqtt_client_dowork shall call the Error Callback function with the message MQTT_CLIENT_NO_PING_RESPONSE] */
                if (mqtt_client->timeSincePing > 0 && ((current_ms - mqtt_client->timeSincePing)/1000) > mqtt_client->maxPingRespTime)
                {
                    // We haven't gotten a ping response in the alloted time
                    set_error_callback(mqtt_client, MQTT_CLIENT_NO_PING_RESPONSE);
                    mqtt_client->timeSincePing = 0;
                    mqtt_client->packetSendTimeMs = 0;
                    mqtt_client->packetState = UNKNOWN_TYPE;
                }
                else if ((((current_ms - mqtt_client->packetSendTimeMs) / 1000) + KEEP_ALIVE_BUFFER_SEC) > mqtt_client->keepAliveInterval)
                {
                    /*Codes_SRS_MQTT_CLIENT_07_026: [if keepAliveInternal is > 0 and the send time is greater than the MQTT KeepAliveInterval then it shall construct an MQTT PINGREQ packet.]*/
                    BUFFER_HANDLE pingPacket = mqtt_codec_ping();
                    if (pingPacket != NULL)
                    {
                        size_t size = BUFFER_length(pingPacket);
                        (void)sendPacketItem(mqtt_client, BUFFER_u_char(pingPacket), size);
                        BUFFER_delete(pingPacket);
                        (void)tickcounter_get_current_ms(mqtt_client->packetTickCntr, &mqtt_client->timeSincePing);

                        if (mqtt_client->logTrace)
                        {
                            STRING_HANDLE trace_log = STRING_construct("PINGREQ");
                            log_outgoing_trace(mqtt_client, trace_log);
                            STRING_delete(trace_log);
                        }
                    }
                }
            }
        }
    }
}

void mqtt_client_set_trace(MQTT_CLIENT_HANDLE handle, bool traceOn, bool rawBytesOn)
{
    MQTT_CLIENT* mqtt_client = (MQTT_CLIENT*)handle;
    if (mqtt_client != NULL)
    {
        mqtt_client->logTrace = traceOn;
        mqtt_client->rawBytesTrace = rawBytesOn;
    }
}

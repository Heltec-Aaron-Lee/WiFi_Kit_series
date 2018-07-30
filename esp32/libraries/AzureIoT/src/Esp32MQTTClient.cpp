// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
#include "Esp32MQTTClient.h"
#include "Arduino.h"

#define CONNECT_TIMEOUT_MS 30000
#define CHECK_INTERVAL_MS 5000
#define MQTT_KEEPALIVE_INTERVAL_S 120
#define SEND_EVENT_RETRY_COUNT 2
#define EVENT_TIMEOUT_MS 10000
#define EVENT_CONFIRMED -2
#define EVENT_FAILED -3

static int callbackCounter;
static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = NULL;
static int receiveContext = 0;
static int statusContext = 0;
static int trackingId = 0;
static int currentTrackingId = -1;
static bool clientConnected = false;
static bool resetClient = false;
static CONNECTION_STATUS_CALLBACK _connection_status_callback = NULL;
static SEND_CONFIRMATION_CALLBACK _send_confirmation_callback = NULL;
static MESSAGE_CALLBACK _message_callback = NULL;
static DEVICE_TWIN_CALLBACK _device_twin_callback = NULL;
static DEVICE_METHOD_CALLBACK _device_method_callback = NULL;
static REPORT_CONFIRMATION_CALLBACK _report_confirmation_callback = NULL;
static bool enableDeviceTwin = false;

static unsigned long iothub_check_ms;

static char *iothub_hostname = NULL;
static char *miniSolutionName = NULL;
const uint8_t* deviceConnectionString = NULL;
const char *esp32Version = "0.1.0";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
static void CheckConnection()
{
    if (resetClient)
    {
        LogInfo(">>>Re-connect.");
        // Re-connect the IoT Hub
        Esp32MQTTClient_Close();
        Esp32MQTTClient_Init(deviceConnectionString, enableDeviceTwin);
        resetClient = false;
    }
}

static char *GetHostNameFromConnectionString(char *connectionString)
{
    if (connectionString == NULL)
    {
        return NULL;
    }
    int start = 0;
    int cur = 0;
    bool find = false;
    while (connectionString[cur] > 0)
    {
        if (connectionString[cur] == '=')
        {
            // Check the key
            if (memcmp(&connectionString[start], "HostName", 8) == 0)
            {
                // This is the host name
                find = true;
            }
            start = ++cur;
            // Value
            while (connectionString[cur] > 0)
            {
                if (connectionString[cur] == ';')
                {
                    break;
                }
                cur++;
            }
            if (find && cur - start > 0)
            {
                char *hostname = (char *)malloc(cur - start + 1);
                memcpy(hostname, &connectionString[start], cur - start);
                hostname[cur - start] = 0;
                return hostname;
            }
            start = cur + 1;
        }
        cur++;
    }
    return NULL;
}

static void FreeEventInstance(EVENT_INSTANCE *event)
{
    if (event != NULL)    
    {
        if (event->type == MESSAGE)
        {
            IoTHubMessage_Destroy(event->messageHandle);
        }
        free(event);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Event handlers
static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void *userContextCallback)
{
    clientConnected = false;

    switch (reason)
    {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        if (result == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED)
        {
            resetClient = true;
            LogInfo(">>>Connection status: timeout");
        }
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        if (result == IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED)
        {
            resetClient = true;
            LogInfo(">>>Connection status: disconnected");
        }
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
        {
            clientConnected = true;
            LogInfo(">>>Connection status: connected");
        }
        break;
    }

    if (_connection_status_callback)
    {
        _connection_status_callback(result, reason);
    }
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    EVENT_INSTANCE *event = (EVENT_INSTANCE *)userContextCallback;
    LogInfo(">>>Confirmation[%d] received for message tracking id = %d with result = %s", callbackCounter++, event->trackingId, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));

    if (currentTrackingId == event->trackingId)
    {
        if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
        {
            currentTrackingId = EVENT_CONFIRMED;
        }
        else
        {
            currentTrackingId = EVENT_FAILED;
        }
    }

    // Free the message
    FreeEventInstance(event);

    if (_send_confirmation_callback)
    {
        _send_confirmation_callback(result);
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
    int *counter = (int *)userContextCallback;
    const char *buffer;
    size_t size;

    // Message content
    if (IoTHubMessage_GetByteArray(message, (const unsigned char **)&buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        LogError("unable to retrieve the message data");
        return IOTHUBMESSAGE_REJECTED;
    }
    else
    {
        char *temp = (char *)malloc(size + 1);
        if (temp == NULL)
        {
            LogError("Failed to malloc for command");
            return IOTHUBMESSAGE_REJECTED;
        }
        memcpy(temp, buffer, size);
        temp[size] = '\0';
        LogInfo(">>>Received Message [%d], Size=%d Message %s", *counter, (int)size, temp);
        if (_message_callback)
        {
            _message_callback(temp, size);
        }
        free(temp);
    }

    /* Some device specific action code goes here... */
    (*counter)++;
    return IOTHUBMESSAGE_ACCEPTED;
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, size_t size, void *userContextCallback)
{
    if (_device_twin_callback)
    {
        _device_twin_callback(updateState, payLoad, size);
    }
}

static int DeviceMethodCallback(const char *methodName, const unsigned char *payload, size_t size, unsigned char **response, size_t *response_size, void *userContextCallback)
{
    if (_device_method_callback)
    {
        return _device_method_callback(methodName, payload, size, response, (int *)response_size);
    }

    const char *responseMessage = "\"No method found\"";
    *response_size = strlen(responseMessage);
    *response = (unsigned char *)strdup("\"No method found\"");

    return 404;
}

static void ReportConfirmationCallback(int statusCode, void *userContextCallback)
{
    EVENT_INSTANCE *event = (EVENT_INSTANCE *)userContextCallback;
    LogInfo(">>>Confirmation[%d] received for state tracking id = %d with state code = %d", callbackCounter++, event->trackingId, statusCode);

    if (statusCode == 204)
    {
        if (currentTrackingId == event->trackingId)
        {
            currentTrackingId = EVENT_CONFIRMED;
        }
    }
    else
    {
        LogError("Report confirmation failed with state code %d", statusCode);
    }

    // Free the state
    FreeEventInstance(event);

    if (_report_confirmation_callback)
    {
        _report_confirmation_callback(statusCode);
    }
}

static bool SendEventOnce(EVENT_INSTANCE *event)
{
    if (event == NULL)
    {
        return false;
    }

    if (iotHubClientHandle == NULL)
    {
        FreeEventInstance(event);
        return false;
    }

    event->trackingId = trackingId++;
    currentTrackingId = event->trackingId;

    uint64_t start_ms = millis();

    CheckConnection();
    
    if (event->type == MESSAGE)
    {
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, event->messageHandle, SendConfirmationCallback, event) != IOTHUB_CLIENT_OK)
        {
            LogError("IoTHubClient_LL_SendEventAsync..........FAILED!");
            FreeEventInstance(event);
            return false;
        }
        LogInfo(">>>IoTHubClient_LL_SendEventAsync accepted message for transmission to IoT Hub.");
    }
    else if (event->type == STATE)
    {
        if (IoTHubClient_LL_SendReportedState(iotHubClientHandle, (const unsigned char *)event->stateString, strlen(event->stateString), ReportConfirmationCallback, event) != IOTHUB_CLIENT_OK)
        {
            LogError("IoTHubClient_LL_SendReportedState..........FAILED!");
            FreeEventInstance(event);
            return false;
        }
        LogInfo(">>>IoTHubClient_LL_SendReportedState accepted state for transmission to IoT Hub.");
    }

    while (true)
    {
        IoTHubClient_LL_DoWork(iotHubClientHandle);

        if (currentTrackingId == EVENT_CONFIRMED)
        {
            // IoT Hub got this event
            return true;
        }

        // Check timeout
        int diff = (int)(millis() - start_ms);
        if (diff >= EVENT_TIMEOUT_MS)
        {
            // Time out, reset the client
            LogError("Waiting for send confirmation, time is up %d", diff);
            resetClient = true;
        }

        if (resetClient)
        {
            // resetClient also can be set as true in the IoTHubClient_LL_DoWork
            // Disconnected, re-send the message
            break;
        }
        else
        {
            // Sleep a while
            ThreadAPI_Sleep(100);
        }
    }

    return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MQTT APIs
EVENT_INSTANCE *Esp32MQTTClient_Event_Generate(const char *eventString, EVENT_TYPE type)
{
    if (eventString == NULL)
    {
        return NULL;
    }

    EVENT_INSTANCE *event = (EVENT_INSTANCE *)malloc(sizeof(EVENT_INSTANCE));
    event->type = type;

    if (type == MESSAGE)
    {
        event->messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)eventString, strlen(eventString));
        if (event->messageHandle == NULL)
        {
            LogError("iotHubMessageHandle is NULL!");
            free(event);
            return NULL;
        }
    }
    else if (type == STATE)
    {
        event->stateString = eventString;
    }

    return event;
}

void Esp32MQTTClient_Event_AddProp(EVENT_INSTANCE *message, const char *key, const char *value)
{
    if (message == NULL || key == NULL)
        return;
    MAP_HANDLE propMap = IoTHubMessage_Properties(message->messageHandle);
    Map_AddOrUpdate(propMap, key, value);
}

bool Esp32MQTTClient_Init(const uint8_t* deviceConnString, bool hasDeviceTwin, bool traceOn)
{
    if (iotHubClientHandle != NULL)
    {
        return true;
    }
    enableDeviceTwin = hasDeviceTwin;
    callbackCounter = 0;
    deviceConnectionString = deviceConnString;

    srand((unsigned int)time(NULL));
    trackingId = 0;

    // Create the IoTHub client
    if (platform_init() != 0)
    {
        LogError("Failed to initialize the platform.");
        return false;
    }

    iothub_hostname = GetHostNameFromConnectionString((char *)deviceConnectionString);

    // Create the IoTHub client
    if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString((char *)deviceConnectionString, MQTT_Protocol)) == NULL)
    {
        return false;
    }

    int keepalive = MQTT_KEEPALIVE_INTERVAL_S;
    IoTHubClient_LL_SetOption(iotHubClientHandle, "keepalive", &keepalive);
    IoTHubClient_LL_SetOption(iotHubClientHandle, "logtrace", &traceOn);

    char *product_info = NULL;
    if (miniSolutionName == NULL)
    {
        int len = snprintf(NULL, 0, "IoT_Esp32_%s", esp32Version);
        product_info = (char *)malloc(len + 1);
        snprintf(product_info, len + 1, "IoT_Esp32_%s", esp32Version);
    }
    else
    {
        int len = snprintf(NULL, 0, "IoT_Esp32_%s_%s", esp32Version, miniSolutionName);
        product_info = (char *)malloc(len + 1);
        snprintf(product_info, len + 1, "IoT_Esp32_%s_%s", esp32Version, miniSolutionName);
    }
    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", product_info) != IOTHUB_CLIENT_OK)
    {
        LogError("Failed to set option \"product_info\"");
        free(product_info);
        return false;
    }
    else
    {
        free(product_info);
    }

    // Setting Message call back, so we can receive commands.
    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, &receiveContext) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubClient_LL_SetMessageCallback..........FAILED!");
        return false;
    }

    if (IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientHandle, ConnectionStatusCallback, &statusContext) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubClient_LL_SetConnectionStatusCallback..........FAILED!");
        return false;
    }

    if (enableDeviceTwin)
    {
        if (IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, DeviceTwinCallback, NULL) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed on IoTHubClient_LL_SetDeviceTwinCallback");
            return false;
        }

        if (IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, DeviceMethodCallback, NULL) != IOTHUB_CLIENT_OK)
        {
            LogError("Failed on IoTHubClient_LL_SetDeviceMethodCallback");
            return false;
        }
    }

    iothub_check_ms = millis();

    // Waiting for the confirmation
    unsigned long start_ms = millis();
    while (true)
    {
        IoTHubClient_LL_DoWork(iotHubClientHandle);
        if (clientConnected)
        {
            break;
        }
        int diff = (int)(millis() - start_ms);
        if (diff >= CONNECT_TIMEOUT_MS)
        {
            // Time out, reset the client
            resetClient = true;
            return false;
        }
        ThreadAPI_Sleep(500);
    }

    return true;
}

bool Esp32MQTTClient_SetOption(const char* optionName, const void* value)
{
    if ((iotHubClientHandle == NULL && (strcmp(optionName, OPTION_MINI_SOLUTION_NAME) != 0))
            || optionName == NULL || value == NULL)
    {
        return false;
    }

    if (strcmp(optionName, OPTION_MINI_SOLUTION_NAME) == 0)
    {
        if (miniSolutionName != NULL)
        {
            free(miniSolutionName);
        }
        miniSolutionName = strdup((char *)value);
        return true;
    }
    else if (IoTHubClient_LL_SetOption(iotHubClientHandle, optionName, value) != IOTHUB_CLIENT_OK)
    {
        LogError("Failed to set option \"%s\"", optionName);
        return false;
    }
    else
    {
        return true;
    }
}

bool Esp32MQTTClient_SendEvent(const char *text)
{
    if (text == NULL)
    {
        return false;
    }
    for (int i = 0; i < SEND_EVENT_RETRY_COUNT; i++)
    {
        if (SendEventOnce(Esp32MQTTClient_Event_Generate(text, MESSAGE)))
        {
            return true;
        }
    }
    return false;
}

bool Esp32MQTTClient_ReceiveEvent()
{
    CheckConnection();

    int count = receiveContext;
    unsigned long tm = millis();
    while ((int)(millis() - tm) < CHECK_INTERVAL_MS)
    {
        IoTHubClient_LL_DoWork(iotHubClientHandle);
        if (count < receiveContext)
        {
            return true;
        }

        if (resetClient)
        {
            // Disconnected
            return false;
        }

        ThreadAPI_Sleep(500);
    }
    // Timeout
    resetClient = true;
    return false;
}

bool Esp32MQTTClient_ReportState(const char *stateString)
{
    if (stateString == NULL)
    {
        return false;
    }
    for (int i = 0; i < SEND_EVENT_RETRY_COUNT; i++)
    {
        if (SendEventOnce(Esp32MQTTClient_Event_Generate(stateString, STATE)))
        {
            return true;
        }
    }
    return false;
}

bool Esp32MQTTClient_SendEventInstance(EVENT_INSTANCE *event)
{
    if (event == NULL)
    {
        return false;
    }

    return SendEventOnce(event);
}

void Esp32MQTTClient_Check(bool hasDelay)
{
    if (iotHubClientHandle == NULL)
    {
        return;
    }

    int diff = hasDelay ? ((int)(millis() - iothub_check_ms)) : CHECK_INTERVAL_MS;
    if (diff >= CHECK_INTERVAL_MS)
    {
        CheckConnection();
        for (int i = 0; i < 5; i++)
        {
            IoTHubClient_LL_DoWork(iotHubClientHandle);
            if (resetClient)
            {
                // Disconnected
                break;
            }
        }
        iothub_check_ms = millis();
    }
}

void Esp32MQTTClient_Close(void)
{
    if (iotHubClientHandle != NULL)
    {
        IoTHubClient_LL_Destroy(iotHubClientHandle);
        iotHubClientHandle = NULL;
    }
}

void Esp32MQTTClient_SetConnectionStatusCallback(CONNECTION_STATUS_CALLBACK connection_status_callback)
{
    _connection_status_callback = connection_status_callback;
}

void Esp32MQTTClient_SetSendConfirmationCallback(SEND_CONFIRMATION_CALLBACK send_confirmation_callback)
{
    _send_confirmation_callback = send_confirmation_callback;
}

void Esp32MQTTClient_SetMessageCallback(MESSAGE_CALLBACK message_callback)
{
    _message_callback = message_callback;
}

void Esp32MQTTClient_SetDeviceTwinCallback(DEVICE_TWIN_CALLBACK device_twin_callback)
{
    _device_twin_callback = device_twin_callback;
}

void Esp32MQTTClient_SetDeviceMethodCallback(DEVICE_METHOD_CALLBACK device_method_callback)
{
    _device_method_callback = device_method_callback;
}

void Esp32MQTTClient_SetReportConfirmationCallback(REPORT_CONFIRMATION_CALLBACK report_confirmation_callback)
{
    _report_confirmation_callback = report_confirmation_callback;
}

void Esp32MQTTClient_Reset(void)
{
    resetClient = true;
    CheckConnection();
}

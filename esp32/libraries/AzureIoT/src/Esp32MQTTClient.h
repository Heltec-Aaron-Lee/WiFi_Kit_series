// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 

#ifndef __IOTHUB_MQTT_CLIENT_H__
#define __IOTHUB_MQTT_CLIENT_H__

#include <stdlib.h>
#include "AzureIotHub.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OPTION_MINI_SOLUTION_NAME "MiniSolution"

enum EVENT_TYPE
{
    MESSAGE, STATE
};

typedef struct EVENT_INSTANCE_TAG
{
    EVENT_TYPE type;
    IOTHUB_MESSAGE_HANDLE messageHandle;
    const char* stateString;
    int trackingId; // For tracking the events within the user callback.
} EVENT_INSTANCE;

/**
* @brief    Generate an event with the event string specified by @p eventString.
*
* @param    eventString             The string of event.
*
* @return   EVENT_INSTANCE upon success or an error code upon failure.
*/
EVENT_INSTANCE* Esp32MQTTClient_Event_Generate(const char *eventString, EVENT_TYPE type);

/**
* @brief    Add new property value for message.
*
* @param    message                 The message need to be modified.
* @param    key                     The property name.
* @param    value                   The property value.
*/
void Esp32MQTTClient_Event_AddProp(EVENT_INSTANCE *message, const char * key, const char * value);

/**
* @brief    Initialize a IoT Hub MQTT client for communication with an existing IoT hub.
*           The connection string is load from the EEPROM.
* @param    deviceConnString   Device connection string.
* @param    hasDeviceTwin   Enable / disable device twin, default is disable.
* @param    traceOn         Enable / disable IoT Hub trace, default is disable.
*
* @return   Return true if initialize successfully, or false if fails.
*/
bool Esp32MQTTClient_Init(const uint8_t* deviceConnString, bool hasDeviceTwin = false, bool traceOn = false);

/**
* @brief    This API sets a runtime option identified by parameter @p optionName
*           to a value pointed to by @p value. @p optionName and the data type
*           @p value is pointing to are specific for every option.
*
* @param    optionName              Name of the option.
*
* @param    value                   The value.
*
* @return   Return true if set option successfully, or false if fails.
*/
bool Esp32MQTTClient_SetOption(const char* optionName, const void* value);

/**
* @brief    Asynchronous call to send the message specified by @p text.
*
* @param    text                The text message.
*
* @return   Return true if send successfully, or false if fails.
*/
bool Esp32MQTTClient_SendEvent(const char *text);

/**
* @brief    Synchronous call to report the state specified by @p stateString.
*
* @param    stateString         The JSON string of reported state.
*
* @return   Return true if send successfully, or false if fails.
*/
bool Esp32MQTTClient_ReportState(const char *stateString);

/**
* @brief    Synchronous call to report the event specified by @p event.
*
* @param    event               The event instance.
*
* @return   Return true if send successfully, or false if fails.
*/
bool Esp32MQTTClient_SendEventInstance(EVENT_INSTANCE *event);

/**
* @brief    Retrieve a message from IoT hub
*
* @return   Return true if get a message successfully, or false if there is no message returns.
*/
bool Esp32MQTTClient_ReceiveEvent();

/**
* @brief    The function is called to try receiving message from IoT hub.
*
* @param    hasDelay        Indicate whether check with IoT hub immediately or has delay, default is delay check (true).
*/
void Esp32MQTTClient_Check(bool hasDelay = true);

/**
* @brief    Disposes of resources allocated by the IoT Hub client.
*/
void Esp32MQTTClient_Close(void);

/**
* @brief    Sets up connection status callback to be invoked representing the status of the connection to IOT Hub.
*/
void Esp32MQTTClient_SetConnectionStatusCallback(CONNECTION_STATUS_CALLBACK connection_status_callback);

/**
* @brief    Sets up send confirmation status callback to be invoked representing the status of sending message to IOT Hub.
*/
void Esp32MQTTClient_SetSendConfirmationCallback(SEND_CONFIRMATION_CALLBACK send_confirmation_callback);

/**
* @brief    Sets up the message callback to be invoked when IoT Hub issues a message to the device.
*/
void Esp32MQTTClient_SetMessageCallback(MESSAGE_CALLBACK message_callback);

/**
* @brief    Sets up the device twin callback to be invoked when IoT Hub update device twin of the device.
*/
void Esp32MQTTClient_SetDeviceTwinCallback(DEVICE_TWIN_CALLBACK device_twin_callback);

/**
* @brief    Sets up the device method callback to be invoked when IoT Hub call method on the device.
*/
void Esp32MQTTClient_SetDeviceMethodCallback(DEVICE_METHOD_CALLBACK device_method_callback);

/**
* @brief    Sets up the report confirmation callback to be invoked when report of the device's properties.
*/
void Esp32MQTTClient_SetReportConfirmationCallback(REPORT_CONFIRMATION_CALLBACK report_confirmation_callback);

/**
* @brief    Force reset the connection.
*/
void Esp32MQTTClient_Reset(void);


#ifdef __cplusplus
}
#endif

#endif /* __IOTHUB_MQTT_CLIENT_H__ */

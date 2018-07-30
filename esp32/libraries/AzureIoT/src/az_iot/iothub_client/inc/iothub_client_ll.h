// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub_client_ll.h
*	@brief	 APIs that allow a user (usually a device) to communicate
*		     with an Azure IoTHub.
*
*	@details IoTHubClient_LL is a module that allows a user (usually a
*			 device) to communicate with an Azure IoTHub. It can send events
*			 and receive messages. At any given moment in time there can only
*			 be at most 1 message callback function.
*
*			 This API surface contains a set of APIs that allows the user to
*			 interact with the lower layer portion of the IoTHubClient. These APIs
*			 contain @c _LL_ in their name, but retain the same functionality like the
*			 @c IoTHubClient_... APIs, with one difference. If the @c _LL_ APIs are
*			 used then the user is responsible for scheduling when the actual work done
*			 by the IoTHubClient happens (when the data is sent/received on/from the wire).
*			 This is useful for constrained devices where spinning a separate thread is
*			 often not desired.
*/

#ifndef IOTHUB_CLIENT_LL_H
#define IOTHUB_CLIENT_LL_H

#include "az_iot/c-utility/inc/azure_c_shared_utility/macro_utils.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"

#define IOTHUB_CLIENT_RESULT_VALUES       \
    IOTHUB_CLIENT_OK,                     \
    IOTHUB_CLIENT_INVALID_ARG,            \
    IOTHUB_CLIENT_ERROR,                  \
    IOTHUB_CLIENT_INVALID_SIZE,           \
    IOTHUB_CLIENT_INDEFINITE_TIME

/** @brief Enumeration specifying the status of calls to various APIs in this module.
*/

DEFINE_ENUM(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

#define IOTHUB_CLIENT_RETRY_POLICY_VALUES     \
    IOTHUB_CLIENT_RETRY_NONE,                   \
    IOTHUB_CLIENT_RETRY_IMMEDIATE,                  \
    IOTHUB_CLIENT_RETRY_INTERVAL,      \
    IOTHUB_CLIENT_RETRY_LINEAR_BACKOFF,      \
    IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF,                 \
    IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER,                 \
    IOTHUB_CLIENT_RETRY_RANDOM

/** @brief Enumeration passed in by the IoT Hub when the event confirmation
*		   callback is invoked to indicate status of the event processing in
*		   the hub.
*/
DEFINE_ENUM(IOTHUB_CLIENT_RETRY_POLICY, IOTHUB_CLIENT_RETRY_POLICY_VALUES);

struct IOTHUBTRANSPORT_CONFIG_TAG;
typedef struct IOTHUBTRANSPORT_CONFIG_TAG IOTHUBTRANSPORT_CONFIG;

typedef struct IOTHUB_CLIENT_LL_HANDLE_DATA_TAG* IOTHUB_CLIENT_LL_HANDLE;

#define IOTHUB_CLIENT_STATUS_VALUES       \
    IOTHUB_CLIENT_SEND_STATUS_IDLE,       \
    IOTHUB_CLIENT_SEND_STATUS_BUSY

/** @brief Enumeration returned by the ::IoTHubClient_LL_GetSendStatus
*		   API to indicate the current sending status of the IoT Hub client.
*/
DEFINE_ENUM(IOTHUB_CLIENT_STATUS, IOTHUB_CLIENT_STATUS_VALUES);

#define IOTHUB_IDENTITY_TYPE_VALUE  \
    IOTHUB_TYPE_TELEMETRY,          \
    IOTHUB_TYPE_DEVICE_TWIN,        \
    IOTHUB_TYPE_DEVICE_METHODS
DEFINE_ENUM(IOTHUB_IDENTITY_TYPE, IOTHUB_IDENTITY_TYPE_VALUE);

#define IOTHUB_PROCESS_ITEM_RESULT_VALUE    \
    IOTHUB_PROCESS_OK,                      \
    IOTHUB_PROCESS_ERROR,                   \
    IOTHUB_PROCESS_NOT_CONNECTED,           \
    IOTHUB_PROCESS_CONTINUE
DEFINE_ENUM(IOTHUB_PROCESS_ITEM_RESULT, IOTHUB_PROCESS_ITEM_RESULT_VALUE);

#define IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES \
    IOTHUBMESSAGE_ACCEPTED, \
    IOTHUBMESSAGE_REJECTED, \
    IOTHUBMESSAGE_ABANDONED

/** @brief Enumeration returned by the callback which is invoked whenever the
*		   IoT Hub sends a message to the device.
*/
DEFINE_ENUM(IOTHUBMESSAGE_DISPOSITION_RESULT, IOTHUBMESSAGE_DISPOSITION_RESULT_VALUES);

#include "az_iot/c-utility/inc/azure_c_shared_utility/agenttime.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/xio.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/doublylinkedlist.h"
#include "iothub_message.h"
#include "iothub_transport_ll.h"
#include "iothub_client_authorization.h"
#include <stddef.h>
#include <stdint.h>

#define IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_VALUES \
    IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_SUCCESS,   \
    IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_ERROR      \

/** @brief Enumeration returned by remotely executed functions
*/
DEFINE_ENUM(IOTHUB_CLIENT_IOTHUB_METHOD_STATUS, IOTHUB_CLIENT_IOTHUB_METHOD_STATUS_VALUES);

#ifdef __cplusplus
extern "C"
{
#endif

#define IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES     \
    IOTHUB_CLIENT_CONFIRMATION_OK,                   \
    IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY,      \
    IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT,      \
    IOTHUB_CLIENT_CONFIRMATION_ERROR                 \

    /** @brief Enumeration passed in by the IoT Hub when the event confirmation
    *		   callback is invoked to indicate status of the event processing in
    *		   the hub.
    */
    DEFINE_ENUM(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES);

#define IOTHUB_CLIENT_CONNECTION_STATUS_VALUES             \
    IOTHUB_CLIENT_CONNECTION_AUTHENTICATED,                \
    IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED               \


    /** @brief Enumeration passed in by the IoT Hub when the connection status
    *		   callback is invoked to indicate status of the connection in
    *		   the hub.
    */
    DEFINE_ENUM(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_VALUES);

#define IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES      \
    IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN,            \
    IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED,              \
    IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL,               \
    IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED,                \
    IOTHUB_CLIENT_CONNECTION_NO_NETWORK,                   \
    IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR,          \
    IOTHUB_CLIENT_CONNECTION_OK                            \

    /** @brief Enumeration passed in by the IoT Hub when the connection status
    *		   callback is invoked to indicate status of the connection in
    *		   the hub.
    */
    DEFINE_ENUM(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, IOTHUB_CLIENT_CONNECTION_STATUS_REASON_VALUES);

#define TRANSPORT_TYPE_VALUES \
    TRANSPORT_LL, /*LL comes from "LowLevel" */ \
    TRANSPORT_THREADED

    DEFINE_ENUM(TRANSPORT_TYPE, TRANSPORT_TYPE_VALUES);

#define DEVICE_TWIN_UPDATE_STATE_VALUES \
    DEVICE_TWIN_UPDATE_COMPLETE, \
    DEVICE_TWIN_UPDATE_PARTIAL

    DEFINE_ENUM(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);

    typedef void(*IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK)(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback);
    typedef void(*IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK)(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback);
    typedef IOTHUBMESSAGE_DISPOSITION_RESULT (*IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC)(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback);
    typedef const TRANSPORT_PROVIDER*(*IOTHUB_CLIENT_TRANSPORT_PROVIDER)(void);

    typedef void(*IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK)(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback);
    typedef void(*IOTHUB_CLIENT_REPORTED_STATE_CALLBACK)(int status_code, void* userContextCallback);
    typedef int(*IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC)(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback);
    typedef int(*IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK)(const char* method_name, const unsigned char* payload, size_t size, METHOD_HANDLE method_id, void* userContextCallback);

    /** @brief	This struct captures IoTHub client configuration. */
    typedef struct IOTHUB_CLIENT_CONFIG_TAG
    {
        /** @brief A function pointer that is passed into the @c IoTHubClientCreate.
        *	A function definition for AMQP is defined in the include @c iothubtransportamqp.h.
        *   A function definition for HTTP is defined in the include @c iothubtransporthttp.h
        *   A function definition for MQTT is defined in the include @c iothubtransportmqtt.h */
        IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;

        /** @brief	A string that identifies the device. */
        const char* deviceId;

        /** @brief	The device key used to authenticate the device. 
        If neither deviceSasToken nor deviceKey is present then the authentication is assumed x509.*/
        const char* deviceKey;

        /** @brief	The device SAS Token used to authenticate the device in place of device key. 
        If neither deviceSasToken nor deviceKey is present then the authentication is assumed x509.*/
        const char* deviceSasToken;

        /** @brief	The IoT Hub name to which the device is connecting. */
        const char* iotHubName;

        /** @brief	IoT Hub suffix goes here, e.g., private.azure-devices-int.net. */
        const char* iotHubSuffix;

        const char* protocolGatewayHostName;
    } IOTHUB_CLIENT_CONFIG;

    /** @brief	This struct captures IoTHub client device configuration. */
    typedef struct IOTHUB_CLIENT_DEVICE_CONFIG_TAG
    {
        /** @brief A function pointer that is passed into the @c IoTHubClientCreate.
        *	A function definition for AMQP is defined in the include @c iothubtransportamqp.h.
        *   A function definition for HTTP is defined in the include @c iothubtransporthttp.h
        *   A function definition for MQTT is defined in the include @c iothubtransportmqtt.h */
        IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;

        /** @brief a transport handle implementing the protocol */
        void * transportHandle;

        /** @brief	A string that identifies the device. */
        const char* deviceId;

        /** @brief	The device key used to authenticate the device. 
        x509 authentication is is not supported for multiplexed connections*/
        const char* deviceKey;

        /** @brief	The device SAS Token used to authenticate the device in place of device key. 
        x509 authentication is is not supported for multiplexed connections.*/
        const char* deviceSasToken;
    } IOTHUB_CLIENT_DEVICE_CONFIG;

    /** @brief	This struct captures IoTHub transport configuration. */
    struct IOTHUBTRANSPORT_CONFIG_TAG
    {
        const IOTHUB_CLIENT_CONFIG* upperConfig;
        PDLIST_ENTRY waitingToSend;
        IOTHUB_AUTHORIZATION_HANDLE auth_module_handle;
    };


    /**
    * @brief	Creates a IoT Hub client for communication with an existing
    * 			IoT Hub using the specified connection string parameter.
    *
    * @param	connectionString	Pointer to a character string
    * @param	protocol			Function pointer for protocol implementation
    *
    *			Sample connection string:
    *				<blockquote>
    *					<pre>HostName=[IoT Hub name goes here].[IoT Hub suffix goes here, e.g., private.azure-devices-int.net];DeviceId=[Device ID goes here];SharedAccessKey=[Device key goes here];</pre>
    *				</blockquote>
    *
    * @return	A non-NULL @c IOTHUB_CLIENT_LL_HANDLE value that is used when
    * 			invoking other functions for IoT Hub client and @c NULL on failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_LL_HANDLE, IoTHubClient_LL_CreateFromConnectionString, const char*, connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER, protocol);

    /**
    * @brief	Creates a IoT Hub client for communication with an existing IoT
    * 			Hub using the specified parameters.
    *
    * @param	config	Pointer to an @c IOTHUB_CLIENT_CONFIG structure
    *
    *			The API does not allow sharing of a connection across multiple
    *			devices. This is a blocking call.
    *
    * @return	A non-NULL @c IOTHUB_CLIENT_LL_HANDLE value that is used when
    * 			invoking other functions for IoT Hub client and @c NULL on failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_LL_HANDLE, IoTHubClient_LL_Create, const IOTHUB_CLIENT_CONFIG*, config);

    /**
    * @brief	Creates a IoT Hub client for communication with an existing IoT
    * 			Hub using an existing transport.
    *
    * @param	config	Pointer to an @c IOTHUB_CLIENT_DEVICE_CONFIG structure
    *
    *			The API *allows* sharing of a connection across multiple
    *			devices. This is a blocking call.
    *
    * @return	A non-NULL @c IOTHUB_CLIENT_LL_HANDLE value that is used when
    * 			invoking other functions for IoT Hub client and @c NULL on failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_LL_HANDLE, IoTHubClient_LL_CreateWithTransport, const IOTHUB_CLIENT_DEVICE_CONFIG*, config);

    /**
    * @brief	Disposes of resources allocated by the IoT Hub client. This is a
    * 			blocking call.
    *
    * @param	iotHubClientHandle	The handle created by a call to the create function.
    */
     MOCKABLE_FUNCTION(, void, IoTHubClient_LL_Destroy, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle);

    /**
    * @brief	Asynchronous call to send the message specified by @p eventMessageHandle.
    *
    * @param	iotHubClientHandle		   	The handle created by a call to the create function.
    * @param	eventMessageHandle		   	The handle to an IoT Hub message.
    * @param	eventConfirmationCallback  	The callback specified by the device for receiving
    * 										confirmation of the delivery of the IoT Hub message.
    * 										This callback can be expected to invoke the
    * 										::IoTHubClient_LL_SendEventAsync function for the
    * 										same message in an attempt to retry sending a failing
    * 										message. The user can specify a @c NULL value here to
    * 										indicate that no callback is required.
    * @param	userContextCallback			User specified context that will be provided to the
    * 										callback. This can be @c NULL.
    *
    *			@b NOTE: The application behavior is undefined if the user calls
    *			the ::IoTHubClient_LL_Destroy function from within any callback.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SendEventAsync, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_MESSAGE_HANDLE, eventMessageHandle, IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK, eventConfirmationCallback, void*, userContextCallback);

    /**
    * @brief	This function returns the current sending status for IoTHubClient.
    *
    * @param	iotHubClientHandle		The handle created by a call to the create function.
    * @param	iotHubClientStatus		The sending state is populated at the address pointed
    * 									at by this parameter. The value will be set to
    * 									@c IOTHUBCLIENT_SENDSTATUS_IDLE if there is currently
    * 								    no item to be sent and @c IOTHUBCLIENT_SENDSTATUS_BUSY
    * 								    if there are.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_GetSendStatus, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_STATUS*, iotHubClientStatus);

    /**
    * @brief	Sets up the message callback to be invoked when IoT Hub issues a
    * 			message to the device. This is a blocking call.
    *
    * @param	iotHubClientHandle		   	The handle created by a call to the create function.
    * @param	messageCallback     	   	The callback specified by the device for receiving
    * 										messages from IoT Hub.
    * @param	userContextCallback			User specified context that will be provided to the
    * 										callback. This can be @c NULL.
    *
    *			@b NOTE: The application behavior is undefined if the user calls
    *			the ::IoTHubClient_LL_Destroy function from within any callback.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SetMessageCallback, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC, messageCallback, void*, userContextCallback);

    /**
    * @brief	Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param	iotHubClientHandle		   	        The handle created by a call to the create function.
    * @param	connectionStatusCallback     	   	The callback specified by the device for receiving
    * 										        updates about the status of the connection to IoT Hub.
    * @param	userContextCallback			        User specified context that will be provided to the
    * 										        callback. This can be @c NULL.
    *
    *			@b NOTE: The application behavior is undefined if the user calls
    *			the ::IoTHubClient_LL_Destroy function from within any callback.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SetConnectionStatusCallback, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK, connectionStatusCallback, void*, userContextCallback);

    /**
    * @brief	Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param	iotHubClientHandle		   	        The handle created by a call to the create function.
    * @param	retryPolicy                  	   	The policy to use to reconnect to IoT Hub when a
    *                                               connection drops.
    * @param	retryTimeoutLimitInSeconds			Maximum amount of time(seconds) to attempt reconnection when a
    *                                               connection drops to IOT Hub.
    *
    *			@b NOTE: The application behavior is undefined if the user calls
    *			the ::IoTHubClient_LL_Destroy function from within any callback.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SetRetryPolicy, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY, retryPolicy, size_t, retryTimeoutLimitInSeconds);


    /**
    * @brief	Sets up the connection status callback to be invoked representing the status of
    * the connection to IOT Hub. This is a blocking call.
    *
    * @param	iotHubClientHandle		   	        The handle created by a call to the create function.
    * @param	retryPolicy                  	   	Out parameter containing the policy to use to reconnect to IoT Hub.
    * @param	retryTimeoutLimitInSeconds			Out parameter containing maximum amount of time in seconds to attempt reconnection
                                                    to IOT Hub.
    *
    *			@b NOTE: The application behavior is undefined if the user calls
    *			the ::IoTHubClient_LL_Destroy function from within any callback.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_GetRetryPolicy, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_RETRY_POLICY*, retryPolicy, size_t*, retryTimeoutLimitInSeconds);

    /**
    * @brief	This function returns in the out parameter @p lastMessageReceiveTime
    * 			what was the value of the @c time function when the last message was
    * 			received at the client.
    *
    * @param	iotHubClientHandle				The handle created by a call to the create function.
    * @param	lastMessageReceiveTime  		Out parameter containing the value of @c time function
    * 											when the last message was received.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_GetLastMessageReceiveTime, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, time_t*, lastMessageReceiveTime);

    /**
    * @brief	This function is meant to be called by the user when work
    * 			(sending/receiving) can be done by the IoTHubClient.
    *
    * @param	iotHubClientHandle	The handle created by a call to the create function.
    *
    *			All IoTHubClient interactions (in regards to network traffic
    *			and/or user level callbacks) are the effect of calling this
    *			function and they take place synchronously inside _DoWork.
    */
     MOCKABLE_FUNCTION(, void, IoTHubClient_LL_DoWork, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle);

    /**
    * @brief	This API sets a runtime option identified by parameter @p optionName
    * 			to a value pointed to by @p value. @p optionName and the data type
    * 			@p value is pointing to are specific for every option.
    *
    * @param	iotHubClientHandle	The handle created by a call to the create function.
    * @param	optionName		  	Name of the option.
    * @param	value			  	The value.
    *
    *			The options that can be set via this API are:
    *				- @b timeout - the maximum time in milliseconds a communication is
    *				  allowed to use. @p value is a pointer to an @c unsigned @c int with
    *				  the timeout value in milliseconds. This is only supported for the HTTP
    *				  protocol as of now. When the HTTP protocol uses CURL, the meaning of
    *				  the parameter is <em>total request time</em>. When the HTTP protocol uses
    *				  winhttp, the meaning is the same as the @c dwSendTimeout and
    *				  @c dwReceiveTimeout parameters of the
    *				  <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/aa384116(v=vs.85).aspx">
    *				  WinHttpSetTimeouts</a> API.
    *				- @b CURLOPT_LOW_SPEED_LIMIT - only available for HTTP protocol and only
    *				  when CURL is used. It has the same meaning as CURL's option with the same
    *				  name. @p value is pointer to a long.
    *				- @b CURLOPT_LOW_SPEED_TIME - only available for HTTP protocol and only
    *				  when CURL is used. It has the same meaning as CURL's option with the same
    *				  name. @p value is pointer to a long.
    *				- @b CURLOPT_FORBID_REUSE - only available for HTTP protocol and only
    *				  when CURL is used. It has the same meaning as CURL's option with the same
    *				  name. @p value is pointer to a long.
    *				- @b CURLOPT_FRESH_CONNECT - only available for HTTP protocol and only
    *				  when CURL is used. It has the same meaning as CURL's option with the same
    *				  name. @p value is pointer to a long.
    *				- @b CURLOPT_VERBOSE - only available for HTTP protocol and only
    *				  when CURL is used. It has the same meaning as CURL's option with the same
    *				  name. @p value is pointer to a long.
    *              - @b keepalive - available for MQTT protocol.  Integer value that sets the
    *                interval in seconds when pings are sent to the server.
    *              - @b logtrace - available for MQTT protocol.  Boolean value that turns on and
    *                off the diagnostic logging.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SetOption, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, const char*, optionName, const void*, value);

    /**
    * @brief	This API specifies a call back to be used when the device receives a desired state update.
    *
    * @param	iotHubClientHandle		The handle created by a call to the create function.
    * @param	deviceTwinCallback	    The callback specified by the device client to be used for updating
    *									the desired state. The callback will be called in response to patch 
    *									request send by the IoTHub services. The payload will be passed to the
    *									callback, along with two version numbers:
    *										- Desired:
    *										- LastSeenReported:
    * @param	userContextCallback		User specified context that will be provided to the
    * 									callback. This can be @c NULL.
    *
    *			@b NOTE: The application behavior is undefined if the user calls
    *			the ::IoTHubClient_LL_Destroy function from within any callback.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SetDeviceTwinCallback, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK, deviceTwinCallback, void*, userContextCallback);

    /**
    * @brief	This API sneds a report of the device's properties and their current values.
    *
    * @param	iotHubClientHandle		The handle created by a call to the create function.
    * @param	reportedState			The current device property values to be 'reported' to the IoTHub.
    * @param	reportedStateCallback	The callback specified by the device client to be called with the
    *									result of the transaction.
    * @param	userContextCallback		User specified context that will be provided to the
    * 									callback. This can be @c NULL.
    *
    *			@b NOTE: The application behavior is undefined if the user calls
    *			the ::IoTHubClient_LL_Destroy function from within any callback.
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SendReportedState, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, const unsigned char*, reportedState, size_t, size, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, reportedStateCallback, void*, userContextCallback);

     /**
     * @brief	This API sets callback for cloud to device method call.
     *
     * @param	iotHubClientHandle		The handle created by a call to the create function.
     * @param	deviceMethodCallback	The callback which will be called by IoTHub.
     * @param	userContextCallback		User specified context that will be provided to the
     * 									callback. This can be @c NULL.
     *
     * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SetDeviceMethodCallback, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC, deviceMethodCallback, void*, userContextCallback);

     /**
     * @brief	This API sets callback for async cloud to device method call.
     *
     * @param	iotHubClientHandle		        The handle created by a call to the create function.
     * @param	inboundDeviceMethodCallback     The callback which will be called by IoTHub.
     * @param	userContextCallback		        User specified context that will be provided to the
     * 									        callback. This can be @c NULL.
     *
     * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_SetDeviceMethodCallback_Ex, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, IOTHUB_CLIENT_INBOUND_DEVICE_METHOD_CALLBACK, inboundDeviceMethodCallback, void*, userContextCallback);

     /**
     * @brief	This API responses to a asnyc method callback identified the methodId.
     *
     * @param	iotHubClientHandle      The handle created by a call to the create function.
     * @param	methodId                The methodId of the Device Method callback.
     * @param	response                The response data for the method callback.
     * @param	response_size           The size of the response data buffer.
     * @param	status_response         The status response of the method callback.
     *
     * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
     */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_DeviceMethodResponse, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, METHOD_HANDLE, methodId, const unsigned char*, response, size_t, respSize, int, statusCode);

#ifdef USE_UPLOADTOBLOB
    /**
    * @brief	This API uploads to Azure Storage the content pointed to by @p source having the size @p size
    *           under the blob name devicename/@pdestinationFileName
    *
    * @param	iotHubClientHandle	    The handle created by a call to the create function.
    * @param	destinationFileName     name of the file.
    * @param	source                  pointer to the source for file content (can be NULL)
    * @param    size                    the size of the source in memory (if @p source is NULL then size needs to be 0).
    *
    * @return	IOTHUB_CLIENT_OK upon success or an error code upon failure.
    */
     MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_LL_UploadToBlob, IOTHUB_CLIENT_LL_HANDLE, iotHubClientHandle, const char*, destinationFileName, const unsigned char*, source, size_t, size);

#endif /*DONT_USE_UPLOADTOBLOB*/

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_LL_H */

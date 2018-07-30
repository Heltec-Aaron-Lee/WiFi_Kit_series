// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file   iothub_message.h
*	@brief  The @c IoTHub_Message component encapsulates one message that
*           can be transferred by an IoT hub client.
*/

#ifndef IOTHUB_MESSAGE_H
#define IOTHUB_MESSAGE_H

#include "az_iot/c-utility/inc/azure_c_shared_utility/macro_utils.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/map.h" 
#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" 
{
#else
#include <stddef.h>
#endif

#define IOTHUB_MESSAGE_RESULT_VALUES         \
    IOTHUB_MESSAGE_OK,                       \
    IOTHUB_MESSAGE_INVALID_ARG,              \
    IOTHUB_MESSAGE_INVALID_TYPE,             \
    IOTHUB_MESSAGE_ERROR                     \

/** @brief Enumeration specifying the status of calls to various  
 *  APIs in this module.
 */
DEFINE_ENUM(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_RESULT_VALUES);

#define IOTHUBMESSAGE_CONTENT_TYPE_VALUES \
IOTHUBMESSAGE_BYTEARRAY, \
IOTHUBMESSAGE_STRING, \
IOTHUBMESSAGE_UNKNOWN \

/** @brief Enumeration specifying the content type of the a given  
  * message.
  */
DEFINE_ENUM(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_CONTENT_TYPE_VALUES);

typedef struct IOTHUB_MESSAGE_HANDLE_DATA_TAG* IOTHUB_MESSAGE_HANDLE;

/**
 * @brief   Creates a new IoT hub message from a byte array. The type of the
 *          message will be set to @c IOTHUBMESSAGE_BYTEARRAY.
 *
 * @param   byteArray   The byte array from which the message is to be created.
 * @param   size        The size of the byte array.
 *
 * @return  A valid @c IOTHUB_MESSAGE_HANDLE if the message was successfully
 *          created or @c NULL in case an error occurs.
 */
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_HANDLE, IoTHubMessage_CreateFromByteArray, const unsigned char*, byteArray, size_t, size);

/**
 * @brief   Creates a new IoT hub message from a null terminated string.  The
 *          type of the message will be set to @c IOTHUBMESSAGE_STRING.
 *
 * @param   source  The null terminated string from which the message is to be
 *                  created.
 *
 * @return  A valid @c IOTHUB_MESSAGE_HANDLE if the message was successfully
 *          created or @c NULL in case an error occurs.
 */
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_HANDLE, IoTHubMessage_CreateFromString, const char*, source);

/**
 * @brief   Creates a new IoT hub message with the content identical to that
 *          of the @p iotHubMessageHandle parameter.
 *
 * @param   iotHubMessageHandle Handle to the message that is to be cloned.
 *
 * @return  A valid @c IOTHUB_MESSAGE_HANDLE if the message was successfully
 *          cloned or @c NULL in case an error occurs.
 */
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_HANDLE, IoTHubMessage_Clone, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);

/**
 * @brief   Fetches a pointer and size for the data associated with the IoT
 *          hub message handle. If the content type of the message is not
 *          @c IOTHUBMESSAGE_BYTEARRAY then the function returns
 *          @c IOTHUB_MESSAGE_INVALID_ARG.
 *
 * @param   iotHubMessageHandle Handle to the message.
 * @param   buffer              Pointer to the memory location where the
 *                              pointer to the buffer will be written.
 * @param   size                The size of the buffer will be written to
 *                              this address.
 *
 * @return  Returns IOTHUB_MESSAGE_OK if the byte array was fetched successfully
 *          or an error code otherwise.
 */
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_RESULT, IoTHubMessage_GetByteArray, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle, const unsigned char**, buffer, size_t*, size);

/**
 * @brief   Returns the null terminated string stored in the message.
 *          If the content type of the message is not @c IOTHUBMESSAGE_STRING
 *          then the function returns @c NULL.
 *
 * @param   iotHubMessageHandle Handle to the message.
 *
 * @return  @c NULL if an error occurs or a pointer to the stored null
 *          terminated string otherwise.
 */
MOCKABLE_FUNCTION(, const char*, IoTHubMessage_GetString, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);

/**
 * @brief   Returns the content type of the message given by parameter
 *          @c iotHubMessageHandle.
 *
 * @param   iotHubMessageHandle Handle to the message.
 *
 * @remarks This function retrieves the standardized type of the payload, which indicates if @c iotHubMessageHandle was created using a String or a Byte Array.
 *
 * @return  An @c IOTHUBMESSAGE_CONTENT_TYPE value.
 */
MOCKABLE_FUNCTION(, IOTHUBMESSAGE_CONTENT_TYPE, IoTHubMessage_GetContentType, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);

/**
* @brief   Sets the content-type of the message payload, as per supported values on RFC 2046.
*
* @param   iotHubMessageHandle Handle to the message.
*
* @param   contentType String defining the type of the payload (e.g., text/plain).
*
* @return  An @c IOTHUB_MESSAGE_RESULT value.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_RESULT, IoTHubMessage_SetContentTypeSystemProperty, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle, const char*, contentType);

/**
* @brief   Returns the content-type of the message payload, if defined.
*
* @param   iotHubMessageHandle Handle to the message.
*
* @return  A string with the content-type value if defined (or NULL otherwise).
*/
MOCKABLE_FUNCTION(, const char*, IoTHubMessage_GetContentTypeSystemProperty, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);

/**
* @brief   Sets the content-encoding of the message payload, as per supported values on RFC 2616.
*
* @param   iotHubMessageHandle Handle to the message.
*
* @param   contentEncoding String defining the encoding of the payload (e.g., utf-8).
*
* @return  An @c IOTHUB_MESSAGE_RESULT value.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_RESULT, IoTHubMessage_SetContentEncodingSystemProperty, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle, const char*, contentEncoding);

/**
* @brief   Returns the content-encoding of the message payload, if defined.
*
* @param   iotHubMessageHandle Handle to the message.
*
* @return  A string with the content-encoding value if defined (or NULL otherwise).
*/
MOCKABLE_FUNCTION(, const char*, IoTHubMessage_GetContentEncodingSystemProperty, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);

/**
 * @brief   Gets a handle to the message's properties map.
 *
 * @param   iotHubMessageHandle Handle to the message.
 *
 * @return  A @c MAP_HANDLE pointing to the properties map for this message.
 */
MOCKABLE_FUNCTION(, MAP_HANDLE, IoTHubMessage_Properties, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);

/**
* @brief   Gets the MessageId from the IOTHUB_MESSAGE_HANDLE.
*
* @param   iotHubMessageHandle Handle to the message.
*
* @return  A const char* pointing to the Message Id.
*/
MOCKABLE_FUNCTION(, const char*, IoTHubMessage_GetMessageId, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);

/**
* @brief   Sets the MessageId for the IOTHUB_MESSAGE_HANDLE.
*
* @param   iotHubMessageHandle Handle to the message.
* @param   messageId Pointer to the memory location of the messageId
*
* @return  Returns IOTHUB_MESSAGE_OK if the messageId was set successfully
*          or an error code otherwise.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_RESULT, IoTHubMessage_SetMessageId, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle, const char*, messageId);

/**
* @brief   Gets the CorrelationId from the IOTHUB_MESSAGE_HANDLE.
*
* @param   iotHubMessageHandle Handle to the message.
*
* @return  A const char* pointing to the Correlation Id.
*/
MOCKABLE_FUNCTION(, const char*, IoTHubMessage_GetCorrelationId, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);

/**
* @brief   Sets the CorrelationId for the IOTHUB_MESSAGE_HANDLE.
*
* @param   iotHubMessageHandle Handle to the message.
* @param   correlationId Pointer to the memory location of the messageId
*
* @return  Returns IOTHUB_MESSAGE_OK if the messageId was set successfully
*          or an error code otherwise.
*/
MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_RESULT, IoTHubMessage_SetCorrelationId, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle, const char*, correlationId);

/**
 * @brief   Frees all resources associated with the given message handle.
 *
 * @param   iotHubMessageHandle Handle to the message.
 */
MOCKABLE_FUNCTION(, void, IoTHubMessage_Destroy, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_MESSAGE_H */

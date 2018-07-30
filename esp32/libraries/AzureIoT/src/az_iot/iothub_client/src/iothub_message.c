// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "az_iot/c-utility/inc/azure_c_shared_utility/optimize_size.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/gballoc.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/xlogging.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/buffer_.h"

#include "../inc/iothub_message.h"

DEFINE_ENUM_STRINGS(IOTHUB_MESSAGE_RESULT, IOTHUB_MESSAGE_RESULT_VALUES);
DEFINE_ENUM_STRINGS(IOTHUBMESSAGE_CONTENT_TYPE, IOTHUBMESSAGE_CONTENT_TYPE_VALUES);

#define LOG_IOTHUB_MESSAGE_ERROR() \
    LogError("(result = %s)", ENUM_TO_STRING(IOTHUB_MESSAGE_RESULT, result));

typedef struct IOTHUB_MESSAGE_HANDLE_DATA_TAG
{
    IOTHUBMESSAGE_CONTENT_TYPE contentType;
    union 
    {
        BUFFER_HANDLE byteArray;
        STRING_HANDLE string;
    } value;
    MAP_HANDLE properties;
    char* messageId;
    char* correlationId;
    char* userDefinedContentType;
    char* contentEncoding;
}IOTHUB_MESSAGE_HANDLE_DATA;

static bool ContainsOnlyUsAscii(const char* asciiValue)
{
    bool result = true;
    const char* iterator = asciiValue;
    while (iterator != NULL && *iterator != '\0')
    {
        // Allow only printable ascii char 
        if (*iterator < ' ' || *iterator > '~')
        {
            result = false;
            break;
        }
        iterator++;
    }
    return result;
}

/* Codes_SRS_IOTHUBMESSAGE_07_008: [ValidateAsciiCharactersFilter shall loop through the mapKey and mapValue strings to ensure that they only contain valid US-Ascii characters Ascii value 32 - 126.] */
static int ValidateAsciiCharactersFilter(const char* mapKey, const char* mapValue)
{
    int result;
    if (!ContainsOnlyUsAscii(mapKey) || !ContainsOnlyUsAscii(mapValue) )
    {
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromByteArray(const unsigned char* byteArray, size_t size)
{
    IOTHUB_MESSAGE_HANDLE_DATA* result;
    if ((byteArray == NULL) && (size != 0))
    {
        LogError("Invalid argument - byteArray is NULL");
        result = NULL;
    }
    else
    {
        result = (IOTHUB_MESSAGE_HANDLE_DATA*)malloc(sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("unable to malloc");
            /*Codes_SRS_IOTHUBMESSAGE_02_024: [If there are any errors then IoTHubMessage_CreateFromByteArray shall return NULL.] */
            /*let it go through*/
        }
        else
        {
            const unsigned char* source;
            unsigned char temp = 0x00;
            if (size != 0)
            {
                /*Codes_SRS_IOTHUBMESSAGE_06_002: [If size is NOT zero then byteArray MUST NOT be NULL*/
                if (byteArray == NULL)
                {
                    LogError("Attempted to create a Hub Message from a NULL pointer!");
                    free(result);
                    result = NULL;
                    source = NULL;
                }
                else
                {
                    source = byteArray;
                }
            }
            else
            {
                /*Codes_SRS_IOTHUBMESSAGE_06_001: [If size is zero then byteArray may be NULL.]*/
                source = &temp;
            }
            if (result != NULL)
            {
                /*Codes_SRS_IOTHUBMESSAGE_02_022: [IoTHubMessage_CreateFromByteArray shall call BUFFER_create passing byteArray and size as parameters.] */
                if ((result->value.byteArray = BUFFER_create(source, size)) == NULL)
                {
                    LogError("BUFFER_create failed");
                    /*Codes_SRS_IOTHUBMESSAGE_02_024: [If there are any errors then IoTHubMessage_CreateFromByteArray shall return NULL.] */
                    free(result);
                    result = NULL;
                }
                /*Codes_SRS_IOTHUBMESSAGE_02_023: [IoTHubMessage_CreateFromByteArray shall call Map_Create to create the message properties.] */
                else if ((result->properties = Map_Create(ValidateAsciiCharactersFilter)) == NULL)
                {
                    LogError("Map_Create failed");
                    /*Codes_SRS_IOTHUBMESSAGE_02_024: [If there are any errors then IoTHubMessage_CreateFromByteArray shall return NULL.] */
                    BUFFER_delete(result->value.byteArray);
                    free(result);
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_IOTHUBMESSAGE_02_025: [Otherwise, IoTHubMessage_CreateFromByteArray shall return a non-NULL handle.] */
                    /*Codes_SRS_IOTHUBMESSAGE_02_026: [The type of the new message shall be IOTHUBMESSAGE_BYTEARRAY.] */
                    result->contentType = IOTHUBMESSAGE_BYTEARRAY;
                    result->messageId = NULL;
                    result->correlationId = NULL;
                    result->userDefinedContentType = NULL;
                    result->contentEncoding = NULL;
                    /*all is fine, return result*/
                }
            }
        }
    }
    return result;
}

IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromString(const char* source)
{
    IOTHUB_MESSAGE_HANDLE_DATA* result;
    if (source == NULL)
    {
        LogError("Invalid argument - source is NULL");
        result = NULL;
    }
    else
    {
        result = (IOTHUB_MESSAGE_HANDLE_DATA*)malloc(sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("malloc failed");
            /*Codes_SRS_IOTHUBMESSAGE_02_029: [If there are any encountered in the execution of IoTHubMessage_CreateFromString then IoTHubMessage_CreateFromString shall return NULL.] */
            /*let it go through*/
        }
        else
        {
            /*Codes_SRS_IOTHUBMESSAGE_02_027: [IoTHubMessage_CreateFromString shall call STRING_construct passing source as parameter.] */
            if ((result->value.string = STRING_construct(source)) == NULL)
            {
                LogError("STRING_construct failed");
                /*Codes_SRS_IOTHUBMESSAGE_02_029: [If there are any encountered in the execution of IoTHubMessage_CreateFromString then IoTHubMessage_CreateFromString shall return NULL.] */
                free(result);
                result = NULL;
            }
            /*Codes_SRS_IOTHUBMESSAGE_02_028: [IoTHubMessage_CreateFromString shall call Map_Create to create the message properties.] */
            else if ((result->properties = Map_Create(ValidateAsciiCharactersFilter)) == NULL)
            {
                LogError("Map_Create failed");
                /*Codes_SRS_IOTHUBMESSAGE_02_029: [If there are any encountered in the execution of IoTHubMessage_CreateFromString then IoTHubMessage_CreateFromString shall return NULL.] */
                STRING_delete(result->value.string);
                free(result);
                result = NULL;
            }
            else
            {
                /*Codes_SRS_IOTHUBMESSAGE_02_031: [Otherwise, IoTHubMessage_CreateFromString shall return a non-NULL handle.] */
                /*Codes_SRS_IOTHUBMESSAGE_02_032: [The type of the new message shall be IOTHUBMESSAGE_STRING.] */
                result->contentType = IOTHUBMESSAGE_STRING;
                result->messageId = NULL;
                result->correlationId = NULL;
                result->userDefinedContentType = NULL;
                result->contentEncoding = NULL;
            }
        }
    }
    return result;
}

/*Codes_SRS_IOTHUBMESSAGE_03_001: [IoTHubMessage_Clone shall create a new IoT hub message with data content identical to that of the iotHubMessageHandle parameter.]*/
IOTHUB_MESSAGE_HANDLE IoTHubMessage_Clone(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    IOTHUB_MESSAGE_HANDLE_DATA* result;
    const IOTHUB_MESSAGE_HANDLE_DATA* source = (const IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;
    /* Codes_SRS_IOTHUBMESSAGE_03_005: [IoTHubMessage_Clone shall return NULL if iotHubMessageHandle is NULL.] */
    if (source == NULL)
    {
        result = NULL;
        LogError("iotHubMessageHandle parameter cannot be NULL for IoTHubMessage_Clone");
    }
    else
    {
        result = (IOTHUB_MESSAGE_HANDLE_DATA*)malloc(sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
        /*Codes_SRS_IOTHUBMESSAGE_03_004: [IoTHubMessage_Clone shall return NULL if it fails for any reason.]*/
        if (result == NULL)
        {
            /*Codes_SRS_IOTHUBMESSAGE_03_004: [IoTHubMessage_Clone shall return NULL if it fails for any reason.]*/
            /*do nothing and return as is*/
            LogError("unable to malloc");
        }
        else
        {
            result->messageId = NULL;
            result->correlationId = NULL;
            result->userDefinedContentType = NULL;
            result->contentEncoding = NULL;

            if (source->messageId != NULL && mallocAndStrcpy_s(&result->messageId, source->messageId) != 0)
            {
                LogError("unable to Copy messageId");
                free(result);
                result = NULL;
            }
            else if (source->correlationId != NULL && mallocAndStrcpy_s(&result->correlationId, source->correlationId) != 0)
            {
                LogError("unable to Copy correlationId");
                if (result->messageId != NULL)
                {
                    free(result->messageId);
                }
                free(result);
                result = NULL;
            }
            else if (source->userDefinedContentType != NULL && mallocAndStrcpy_s(&result->userDefinedContentType, source->userDefinedContentType) != 0)
            {
                LogError("unable to copy contentType");
                if (result->messageId != NULL)
                {
                    free(result->messageId);
                }
                if (result->correlationId != NULL)
                {
                    free(result->correlationId);
                }
                free(result);
                result = NULL;
            }
            else if (source->contentEncoding != NULL && mallocAndStrcpy_s(&result->contentEncoding, source->contentEncoding) != 0)
            {
                LogError("unable to copy contentEncoding");
                if (result->messageId != NULL)
                {
                    free(result->messageId);
                }
                if (result->correlationId != NULL)
                {
                    free(result->correlationId);
                }
                if (result->userDefinedContentType != NULL)
                {
                    free(result->userDefinedContentType);
                }
                free(result);
                result = NULL;
            }
            else if (source->contentType == IOTHUBMESSAGE_BYTEARRAY)
            {
                /*Codes_SRS_IOTHUBMESSAGE_02_006: [IoTHubMessage_Clone shall clone to content by a call to BUFFER_clone] */
                if ((result->value.byteArray = BUFFER_clone(source->value.byteArray)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMESSAGE_03_004: [IoTHubMessage_Clone shall return NULL if it fails for any reason.]*/
                    LogError("unable to BUFFER_clone");
                    if (result->messageId)
                    {
                        free(result->messageId);
                        result->messageId = NULL;
                    }
                    if (result->correlationId != NULL)
                    {
                        free(result->correlationId);
                        result->correlationId = NULL;
                    }
                    free(result);
                    result = NULL;
                }
                /*Codes_SRS_IOTHUBMESSAGE_02_005: [IoTHubMessage_Clone shall clone the properties map by using Map_Clone.] */
                else if ((result->properties = Map_Clone(source->properties)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMESSAGE_03_004: [IoTHubMessage_Clone shall return NULL if it fails for any reason.]*/
                    LogError("unable to Map_Clone");
                    BUFFER_delete(result->value.byteArray);
                    if (result->messageId)
                    {
                        free(result->messageId);
                        result->messageId = NULL;
                    }
                    if (result->correlationId != NULL)
                    {
                        free(result->correlationId);
                        result->correlationId = NULL;
                    }
                    free(result);
                    result = NULL;
                }
                else
                {
                    result->contentType = IOTHUBMESSAGE_BYTEARRAY;
                    /*Codes_SRS_IOTHUBMESSAGE_03_002: [IoTHubMessage_Clone shall return upon success a non-NULL handle to the newly created IoT hub message.]*/
                    /*return as is, this is a good result*/
                }
            }
            else /*can only be STRING*/
            {
                /*Codes_SRS_IOTHUBMESSAGE_02_006: [IoTHubMessage_Clone shall clone the content by a call to BUFFER_clone or STRING_clone] */
                if ((result->value.string = STRING_clone(source->value.string)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMESSAGE_03_004: [IoTHubMessage_Clone shall return NULL if it fails for any reason.]*/
                    if (result->messageId)
                    {
                        free(result->messageId);
                        result->messageId = NULL;
                    }
                    if (result->correlationId != NULL)
                    {
                        free(result->correlationId);
                        result->correlationId = NULL;
                    }
                    free(result);
                    result = NULL;
                    LogError("failed to STRING_clone");
                }
                /*Codes_SRS_IOTHUBMESSAGE_02_005: [IoTHubMessage_Clone shall clone the properties map by using Map_Clone.] */
                else if ((result->properties = Map_Clone(source->properties)) == NULL)
                {
                    /*Codes_SRS_IOTHUBMESSAGE_03_004: [IoTHubMessage_Clone shall return NULL if it fails for any reason.]*/
                    LogError("unable to Map_Clone");
                    STRING_delete(result->value.string);
                    if (result->messageId)
                    {
                        free(result->messageId);
                        result->messageId = NULL;
                    }
                    if (result->correlationId != NULL)
                    {
                        free(result->correlationId);
                        result->correlationId = NULL;
                    }
                    free(result);
                    result = NULL;
                }
                else
                {
                    result->contentType = IOTHUBMESSAGE_STRING;
                    /*all is fine*/
                }
            }
        }
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_GetByteArray(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const unsigned char** buffer, size_t* size)
{
    IOTHUB_MESSAGE_RESULT result;
    if (
        (iotHubMessageHandle == NULL) ||
        (buffer == NULL) ||
        (size == NULL)
        )
    {
        /*Codes_SRS_IOTHUBMESSAGE_01_014: [If any of the arguments passed to IoTHubMessage_GetByteArray  is NULL IoTHubMessage_GetByteArray shall return IOTHUBMESSAGE_INVALID_ARG.] */
        LogError("invalid parameter (NULL) to IoTHubMessage_GetByteArray IOTHUB_MESSAGE_HANDLE iotHubMessageHandle=%p, const unsigned char** buffer=%p, size_t* size=%p", iotHubMessageHandle, buffer, size);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        if (handleData->contentType != IOTHUBMESSAGE_BYTEARRAY)
        {
            /*Codes_SRS_IOTHUBMESSAGE_02_021: [If iotHubMessageHandle is not a iothubmessage containing BYTEARRAY data, then IoTHubMessage_GetData shall write in *buffer NULL and shall set *size to 0.] */
            result = IOTHUB_MESSAGE_INVALID_ARG;
            LogError("invalid type of message %s", ENUM_TO_STRING(IOTHUBMESSAGE_CONTENT_TYPE, handleData->contentType));
        }
        else
        {
            /*Codes_SRS_IOTHUBMESSAGE_01_011: [The pointer shall be obtained by using BUFFER_u_char and it shall be copied in the buffer argument.]*/
            *buffer = BUFFER_u_char(handleData->value.byteArray);
            /*Codes_SRS_IOTHUBMESSAGE_01_012: [The size of the associated data shall be obtained by using BUFFER_length and it shall be copied to the size argument.]*/
            *size = BUFFER_length(handleData->value.byteArray);
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

const char* IoTHubMessage_GetString(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    if (iotHubMessageHandle == NULL)
    {
        /*Codes_SRS_IOTHUBMESSAGE_02_016: [If any parameter is NULL then IoTHubMessage_GetString  shall return NULL.] */
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        if (handleData->contentType != IOTHUBMESSAGE_STRING)
        {
            /*Codes_SRS_IOTHUBMESSAGE_02_017: [IoTHubMessage_GetString shall return NULL if the iotHubMessageHandle does not refer to a IOTHUBMESSAGE of type STRING.] */
            result = NULL;
        }
        else
        {
            /*Codes_SRS_IOTHUBMESSAGE_02_018: [IoTHubMessage_GetStringData shall return the currently stored null terminated string.] */
            result = STRING_c_str(handleData->value.string);
        }
    }
    return result;
}

IOTHUBMESSAGE_CONTENT_TYPE IoTHubMessage_GetContentType(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    IOTHUBMESSAGE_CONTENT_TYPE result;
    /*Codes_SRS_IOTHUBMESSAGE_02_008: [If any parameter is NULL then IoTHubMessage_GetContentType shall return IOTHUBMESSAGE_UNKNOWN.] */
    if (iotHubMessageHandle == NULL)
    {
        result = IOTHUBMESSAGE_UNKNOWN;
    }
    else
    {
        /*Codes_SRS_IOTHUBMESSAGE_02_009: [Otherwise IoTHubMessage_GetContentType shall return the type of the message.] */
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        result = handleData->contentType;
    }
    return result;
}

MAP_HANDLE IoTHubMessage_Properties(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    MAP_HANDLE result;
    /*Codes_SRS_IOTHUBMESSAGE_02_001: [If iotHubMessageHandle is NULL then IoTHubMessage_Properties shall return NULL.]*/
    if (iotHubMessageHandle == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_Properties");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_IOTHUBMESSAGE_02_002: [Otherwise, for any non-NULL iotHubMessageHandle it shall return a non-NULL MAP_HANDLE.]*/
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = (IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;
        result = handleData->properties;
    }
    return result;
}

const char* IoTHubMessage_GetCorrelationId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    /* Codes_SRS_IOTHUBMESSAGE_07_016: [if the iotHubMessageHandle parameter is NULL then IoTHubMessage_GetCorrelationId shall return a NULL value.] */
    if (iotHubMessageHandle == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_GetCorrelationId");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_IOTHUBMESSAGE_07_017: [IoTHubMessage_GetCorrelationId shall return the correlationId as a const char*.] */
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        result = handleData->correlationId;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetCorrelationId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* correlationId)
{
    IOTHUB_MESSAGE_RESULT result;
    /* Codes_SRS_IOTHUBMESSAGE_07_018: [if any of the parameters are NULL then IoTHubMessage_SetCorrelationId shall return a IOTHUB_MESSAGE_INVALID_ARG value.]*/
    if (iotHubMessageHandle == NULL || correlationId == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_SetCorrelationId");
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        /* Codes_SRS_IOTHUBMESSAGE_07_019: [If the IOTHUB_MESSAGE_HANDLE correlationId is not NULL, then the IOTHUB_MESSAGE_HANDLE correlationId will be deallocated.] */
        if (handleData->correlationId != NULL)
        {
            free(handleData->correlationId);
            handleData->correlationId = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->correlationId, correlationId) != 0)
        {
            /* Codes_SRS_IOTHUBMESSAGE_07_020: [If the allocation or the copying of the correlationId fails, then IoTHubMessage_SetCorrelationId shall return IOTHUB_MESSAGE_ERROR.] */
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            /* Codes_SRS_IOTHUBMESSAGE_07_021: [IoTHubMessage_SetCorrelationId finishes successfully it shall return IOTHUB_MESSAGE_OK.] */
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* messageId)
{
    IOTHUB_MESSAGE_RESULT result;
    /* Codes_SRS_IOTHUBMESSAGE_07_012: [if any of the parameters are NULL then IoTHubMessage_SetMessageId shall return a IOTHUB_MESSAGE_INVALID_ARG value.] */
    if (iotHubMessageHandle == NULL || messageId == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_SetMessageId");
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        /* Codes_SRS_IOTHUBMESSAGE_07_013: [If the IOTHUB_MESSAGE_HANDLE messageId is not NULL, then the IOTHUB_MESSAGE_HANDLE messageId will be freed] */
        if (handleData->messageId != NULL)
        {
            free(handleData->messageId);
            handleData->messageId = NULL;
        }

        /* Codes_SRS_IOTHUBMESSAGE_07_014: [If the allocation or the copying of the messageId fails, then IoTHubMessage_SetMessageId shall return IOTHUB_MESSAGE_ERROR.] */
        if (mallocAndStrcpy_s(&handleData->messageId, messageId) != 0)
        {
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            result = IOTHUB_MESSAGE_OK;
        }
    }
    return result;
}

const char* IoTHubMessage_GetMessageId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;
    /* Codes_SRS_IOTHUBMESSAGE_07_010: [if the iotHubMessageHandle parameter is NULL then IoTHubMessage_MessageId shall return a NULL value.] */
    if (iotHubMessageHandle == NULL)
    {
        LogError("invalid arg (NULL) passed to IoTHubMessage_GetMessageId");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_IOTHUBMESSAGE_07_011: [IoTHubMessage_MessageId shall return the messageId as a const char*.] */
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        result = handleData->messageId;
    }
    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentTypeSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* contentType)
{
    IOTHUB_MESSAGE_RESULT result;

    // Codes_SRS_IOTHUBMESSAGE_09_001: [If any of the parameters are NULL then IoTHubMessage_SetContentTypeSystemProperty shall return a IOTHUB_MESSAGE_INVALID_ARG value.] 
    if (iotHubMessageHandle == NULL || contentType == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, contentType=%p)", iotHubMessageHandle, contentType);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = (IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;

        // Codes_SRS_IOTHUBMESSAGE_09_002: [If the IOTHUB_MESSAGE_HANDLE `contentType` is not NULL it shall be deallocated.] 
        if (handleData->userDefinedContentType != NULL)
        {
            free(handleData->userDefinedContentType);
            handleData->userDefinedContentType = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->userDefinedContentType, contentType) != 0)
        {
            LogError("Failed saving a copy of contentType");
            // Codes_SRS_IOTHUBMESSAGE_09_003: [If the allocation or the copying of `contentType` fails, then IoTHubMessage_SetContentTypeSystemProperty shall return IOTHUB_MESSAGE_ERROR.] 
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            // Codes_SRS_IOTHUBMESSAGE_09_004: [If IoTHubMessage_SetContentTypeSystemProperty finishes successfully it shall return IOTHUB_MESSAGE_OK.]
            result = IOTHUB_MESSAGE_OK;
        }
    }

    return result;
}

const char* IoTHubMessage_GetContentTypeSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;

    // Codes_SRS_IOTHUBMESSAGE_09_005: [If any of the parameters are NULL then IoTHubMessage_GetContentTypeSystemProperty shall return a IOTHUB_MESSAGE_INVALID_ARG value.] 
    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;

        // Codes_SRS_IOTHUBMESSAGE_09_006: [IoTHubMessage_GetContentTypeSystemProperty shall return the `contentType` as a const char* ] 
        result = (const char*)handleData->userDefinedContentType;
    }

    return result;
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentEncodingSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* contentEncoding)
{
    IOTHUB_MESSAGE_RESULT result;

    // Codes_SRS_IOTHUBMESSAGE_09_006: [If any of the parameters are NULL then IoTHubMessage_SetContentEncodingSystemProperty shall return a IOTHUB_MESSAGE_INVALID_ARG value.] 
    if (iotHubMessageHandle == NULL || contentEncoding == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle=%p, contentEncoding=%p)", iotHubMessageHandle, contentEncoding);
        result = IOTHUB_MESSAGE_INVALID_ARG;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = (IOTHUB_MESSAGE_HANDLE_DATA*)iotHubMessageHandle;

        // Codes_SRS_IOTHUBMESSAGE_09_007: [If the IOTHUB_MESSAGE_HANDLE `contentEncoding` is not NULL it shall be deallocated.] 
        if (handleData->contentEncoding != NULL)
        {
            free(handleData->contentEncoding);
            handleData->contentEncoding = NULL;
        }

        if (mallocAndStrcpy_s(&handleData->contentEncoding, contentEncoding) != 0)
        {
            LogError("Failed saving a copy of contentEncoding");
            // Codes_SRS_IOTHUBMESSAGE_09_008: [If the allocation or the copying of `contentEncoding` fails, then IoTHubMessage_SetContentEncodingSystemProperty shall return IOTHUB_MESSAGE_ERROR.]
            result = IOTHUB_MESSAGE_ERROR;
        }
        else
        {
            // Codes_SRS_IOTHUBMESSAGE_09_009: [If IoTHubMessage_SetContentEncodingSystemProperty finishes successfully it shall return IOTHUB_MESSAGE_OK.]
            result = IOTHUB_MESSAGE_OK;
        }
    }

    return result;
}

const char* IoTHubMessage_GetContentEncodingSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    const char* result;

    // Codes_SRS_IOTHUBMESSAGE_09_010: [If any of the parameters are NULL then IoTHubMessage_GetContentEncodingSystemProperty shall return a IOTHUB_MESSAGE_INVALID_ARG value.] 
    if (iotHubMessageHandle == NULL)
    {
        LogError("Invalid argument (iotHubMessageHandle is NULL)");
        result = NULL;
    }
    else
    {
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;

        // Codes_SRS_IOTHUBMESSAGE_09_011: [IoTHubMessage_GetContentEncodingSystemProperty shall return the `contentEncoding` as a const char* ] 
        result = (const char*)handleData->contentEncoding;
    }

    return result;
}

void IoTHubMessage_Destroy(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
    /*Codes_SRS_IOTHUBMESSAGE_01_004: [If iotHubMessageHandle is NULL, IoTHubMessage_Destroy shall do nothing.] */
    if (iotHubMessageHandle != NULL)
    {
        /*Codes_SRS_IOTHUBMESSAGE_01_003: [IoTHubMessage_Destroy shall free all resources associated with iotHubMessageHandle.]  */
        IOTHUB_MESSAGE_HANDLE_DATA* handleData = iotHubMessageHandle;
        if (handleData->contentType == IOTHUBMESSAGE_BYTEARRAY)
        {
            BUFFER_delete(handleData->value.byteArray);
        }
        else if (handleData->contentType == IOTHUBMESSAGE_STRING)
        {
            STRING_delete(handleData->value.string);
        }
        else
        {
            LogError("Unknown contentType in IoTHubMessage");
        }
        Map_Destroy(handleData->properties);
        free(handleData->messageId);
        handleData->messageId = NULL;
        free(handleData->correlationId);
        handleData->correlationId = NULL;
        free(handleData->userDefinedContentType);
        free(handleData->contentEncoding);
        free(handleData);
    }
}

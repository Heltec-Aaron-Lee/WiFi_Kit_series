// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file blob.h
*	@brief Contains blob APIs needed for File Upload feature of IoTHub client.
*
*	@details IoTHub client needs to upload a byte array by using blob storage API
*			 IoTHub service provides the complete SAS URI to execute a PUT request
*			 that will upload the data.
*			
*/

#ifndef BLOB_H
#define BLOB_H

#include "az_iot/c-utility/inc/azure_c_shared_utility/macro_utils.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/buffer_.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"

#define BLOB_RESULT_VALUES \
    BLOB_OK,               \
    BLOB_ERROR,            \
    BLOB_NOT_IMPLEMENTED,  \
    BLOB_HTTP_ERROR,       \
    BLOB_INVALID_ARG    

DEFINE_ENUM(BLOB_RESULT, BLOB_RESULT_VALUES)

/**
* @brief	Synchronously uploads a byte array to blob storage
*
* @param	SASURI	        The URI to use to upload data
* @param	size		    The size of the data to be uploaded (can be 0)
* @param	source		    A pointer to the byte array to be uploaded (can be NULL, but then size needs to be zero)
* @param    httpStatus      A pointer to an out argument receiving the HTTP status (available only when the return value is BLOB_OK)
* @param    httpResponse    A BUFFER_HANDLE that receives the HTTP response from the server (available only when the return value is BLOB_OK)
* @param    certificates    A null terminated string containing CA certificates to be used
*
* @return	A @c BLOB_RESULT. BLOB_OK means the blob has been uploaded successfully. Any other value indicates an error
*/
MOCKABLE_FUNCTION(, BLOB_RESULT, Blob_UploadFromSasUri,const char*, SASURI, const unsigned char*, source, size_t, size, unsigned int*, httpStatus, BUFFER_HANDLE, httpResponse, const char*, certificates)

#ifdef __cplusplus
}
#endif

#endif /* BLOB_H */

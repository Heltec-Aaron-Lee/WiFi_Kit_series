// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_CLIENT_AUTHORIZATION_H
#define IOTHUB_CLIENT_AUTHORIZATION_H

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif /* __cplusplus */

#include "az_iot/c-utility/inc/azure_c_shared_utility/macro_utils.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/umock_c_prod.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/xio.h"

typedef struct IOTHUB_AUTHORIZATION_DATA_TAG* IOTHUB_AUTHORIZATION_HANDLE;

#define IOTHUB_CREDENTIAL_TYPE_VALUES       \
    IOTHUB_CREDENTIAL_TYPE_UNKNOWN,         \
    IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY,      \
    IOTHUB_CREDENTIAL_TYPE_X509,            \
    IOTHUB_CREDENTIAL_TYPE_X509_ECC,        \
    IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN,       \
    IOTHUB_CREDENTIAL_TYPE_DEVICE_AUTH


DEFINE_ENUM(IOTHUB_CREDENTIAL_TYPE, IOTHUB_CREDENTIAL_TYPE_VALUES);

#define SAS_TOKEN_STATUS_VALUES  \
    SAS_TOKEN_STATUS_FAILED,     \
    SAS_TOKEN_STATUS_VALID,      \
    SAS_TOKEN_STATUS_INVALID

DEFINE_ENUM(SAS_TOKEN_STATUS, SAS_TOKEN_STATUS_VALUES);

MOCKABLE_FUNCTION(, IOTHUB_AUTHORIZATION_HANDLE, IoTHubClient_Auth_Create, const char*, device_key, const char*, device_id, const char*, device_sas_token);
MOCKABLE_FUNCTION(, IOTHUB_AUTHORIZATION_HANDLE, IoTHubClient_Auth_CreateFromDeviceAuth, const char*, device_id);
MOCKABLE_FUNCTION(, void, IoTHubClient_Auth_Destroy, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, IOTHUB_CREDENTIAL_TYPE, IoTHubClient_Auth_Set_x509_Type, IOTHUB_AUTHORIZATION_HANDLE, handle, bool, enable_x509);
MOCKABLE_FUNCTION(, IOTHUB_CREDENTIAL_TYPE, IoTHubClient_Auth_Get_Credential_Type, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, char*, IoTHubClient_Auth_Get_SasToken, IOTHUB_AUTHORIZATION_HANDLE, handle, const char*, scope, size_t, expire_time);
MOCKABLE_FUNCTION(, int, IoTHubClient_Auth_Set_xio_Certificate, IOTHUB_AUTHORIZATION_HANDLE, handle, XIO_HANDLE, xio);
MOCKABLE_FUNCTION(, const char*, IoTHubClient_Auth_Get_DeviceId, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, const char*, IoTHubClient_Auth_Get_DeviceKey, IOTHUB_AUTHORIZATION_HANDLE, handle);
MOCKABLE_FUNCTION(, SAS_TOKEN_STATUS, IoTHubClient_Auth_Is_SasToken_Valid, IOTHUB_AUTHORIZATION_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_AUTHORIZATION_H */

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UNIQUEID_H
#define UNIQUEID_H

#include "macro_utils.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

#include "umock_c_prod.h"

#define UNIQUEID_RESULT_VALUES    \
    UNIQUEID_OK,                  \
    UNIQUEID_INVALID_ARG,         \
    UNIQUEID_ERROR

    DEFINE_ENUM(UNIQUEID_RESULT, UNIQUEID_RESULT_VALUES)

        MOCKABLE_FUNCTION(, UNIQUEID_RESULT, UniqueId_Generate, char*, uid, size_t, bufferSize);

        /*
        *   @brief                  Gets the string representation of the UUID bytes.
        *   @param uid_bytes        Sequence of bytes representing an UUID.
        *   @param uid_size         Number of bytes representing the UUID (default is 16).
        *   @param output_string    Pre-allocated buffer where the UUID will be stored as string (e.g., "7f907d75-5e13-44cf-a1a3-19a01a2b4528").
        *   @returns                UNIQUEID_OK if no errors occur, UNIQUEID_INVALID_ARG otherwise.
        */
        MOCKABLE_FUNCTION(, UNIQUEID_RESULT, UniqueId_GetStringFromBytes, unsigned char*, uid, size_t, uid_size, char*, output_string);

#ifdef __cplusplus
}
#endif

#endif /* UNIQUEID_H */

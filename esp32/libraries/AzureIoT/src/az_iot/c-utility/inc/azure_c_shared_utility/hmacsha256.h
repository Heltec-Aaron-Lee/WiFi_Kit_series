// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HMACSHA256_H
#define HMACSHA256_H

#include "macro_utils.h"
#include "buffer_.h"
#include "umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HMACSHA256_RESULT_VALUES              \
    HMACSHA256_OK,                            \
    HMACSHA256_INVALID_ARG,                   \
    HMACSHA256_ERROR

DEFINE_ENUM(HMACSHA256_RESULT, HMACSHA256_RESULT_VALUES)

MOCKABLE_FUNCTION(, HMACSHA256_RESULT, HMACSHA256_ComputeHash, const unsigned char*, key, size_t, keyLen, const unsigned char*, payload, size_t, payloadLen, BUFFER_HANDLE, hash);

#ifdef __cplusplus
}
#endif

#endif /* HMACSHA256_H */

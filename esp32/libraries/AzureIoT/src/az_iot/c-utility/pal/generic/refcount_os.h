// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This file gets included into refcount.h as a means of extending the behavior of
// atomic increment, decrement, and test. 
//
// The Azure IoT C SDK does not require thread-safe refcount operations, so 
// this file is appropriate for any device when using the Azure IoT C SDK.

#ifndef REFCOUNT_OS_H__GENERIC
#define REFCOUNT_OS_H__GENERIC

#define COUNT_TYPE uint32_t

#define DEC_RETURN_ZERO (0)
#define INC_REF(type, var) ++((((REFCOUNT_TYPE(type)*)var)->count))
#define DEC_REF(type, var) --((((REFCOUNT_TYPE(type)*)var)->count))

#endif // REFCOUNT_OS_H__GENERIC

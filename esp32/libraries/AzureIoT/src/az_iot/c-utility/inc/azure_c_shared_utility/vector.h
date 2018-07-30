// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef VECTOR_H
#define VECTOR_H

#include "crt_abstractions.h"
#include "umock_c_prod.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdbool>
extern "C"
{
#else
#include <stddef.h>
#include <stdbool.h>
#endif

#include "vector_types.h"

/* creation */
MOCKABLE_FUNCTION(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
MOCKABLE_FUNCTION(, VECTOR_HANDLE, VECTOR_move, VECTOR_HANDLE, handle);
MOCKABLE_FUNCTION(, void, VECTOR_destroy, VECTOR_HANDLE, handle);

/* insertion */
MOCKABLE_FUNCTION(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);

/* removal */
MOCKABLE_FUNCTION(, void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, numElements);
MOCKABLE_FUNCTION(, void, VECTOR_clear, VECTOR_HANDLE, handle);

/* access */
MOCKABLE_FUNCTION(, void*, VECTOR_element, VECTOR_HANDLE, handle, size_t, index);
MOCKABLE_FUNCTION(, void*, VECTOR_front, VECTOR_HANDLE, handle);
MOCKABLE_FUNCTION(, void*, VECTOR_back, VECTOR_HANDLE, handle);
MOCKABLE_FUNCTION(, void*, VECTOR_find_if, VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);

/* capacity */
MOCKABLE_FUNCTION(, size_t, VECTOR_size, VECTOR_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /* VECTOR_H */

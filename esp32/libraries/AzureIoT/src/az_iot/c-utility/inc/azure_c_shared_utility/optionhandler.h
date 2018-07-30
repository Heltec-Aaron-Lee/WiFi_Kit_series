// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef OPTIONHANDLER_H
#define OPTIONHANDLER_H

#include "macro_utils.h"

#define OPTIONHANDLER_RESULT_VALUES \
OPTIONHANDLER_OK, \
OPTIONHANDLER_ERROR, \
OPTIONHANDLER_INVALIDARG

DEFINE_ENUM(OPTIONHANDLER_RESULT, OPTIONHANDLER_RESULT_VALUES)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "umock_c_prod.h"

typedef struct OPTIONHANDLER_HANDLE_DATA_TAG* OPTIONHANDLER_HANDLE;

/*the following function pointer points to a function that produces a clone of the option specified by name and value (that is, a clone of void* value)*/
/*returns NULL if it failed to produce a clone, otherwise returns a non-NULL value*/
/*to be implemented by every module*/
typedef void* (*pfCloneOption)(const char* name, const void* value);

/*the following function pointer points to a function that frees resources allocated for an option*/
/*to be implemented by every module*/
typedef void (*pfDestroyOption)(const char* name, const void* value);

/*the following function pointer points to a function that sets an option for a module*/
/*to be implemented by every module*/
/*returns 0 if _SetOption succeeded, any other value is error, if the option is not intended for that module, returns 0*/
typedef int (*pfSetOption)(void* handle, const char* name, const void* value);

MOCKABLE_FUNCTION(, OPTIONHANDLER_HANDLE, OptionHandler_Create, pfCloneOption, cloneOption, pfDestroyOption, destroyOption, pfSetOption, setOption);
MOCKABLE_FUNCTION(, OPTIONHANDLER_HANDLE, OptionHandler_Clone, OPTIONHANDLER_HANDLE, handler);
MOCKABLE_FUNCTION(, OPTIONHANDLER_RESULT, OptionHandler_AddOption, OPTIONHANDLER_HANDLE, handle, const char*, name, const void*, value);
MOCKABLE_FUNCTION(, OPTIONHANDLER_RESULT, OptionHandler_FeedOptions, OPTIONHANDLER_HANDLE, handle, void*, destinationHandle);
MOCKABLE_FUNCTION(, void, OptionHandler_Destroy, OPTIONHANDLER_HANDLE, handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*OPTIONHANDLER*/

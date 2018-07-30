// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef OPTIMIZE_SIZE_H
#define OPTIMIZE_SIZE_H

#if (defined __MSP430FR5969__)
    #define OPTIMIZE_RETURN_CODES
    #define NO_VERBOSE_OUTPUT
    #ifdef DEBUG
        #define MINIMAL_LOGERROR
    #else
        #define NO_LOGGING
    #endif
#endif

#if (defined OPTIMIZE_RETURN_CODES)
    #define __FAILURE__ 1
#else
    #define __FAILURE__ __LINE__
#endif

#endif // OPTIMIZE_SIZE_H


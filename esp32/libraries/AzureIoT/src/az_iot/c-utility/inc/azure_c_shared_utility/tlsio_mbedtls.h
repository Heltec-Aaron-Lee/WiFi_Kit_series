// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TLSIO_MBEDTLS_H
#define TLSIO_MBEDTLS_H

#include "azure_c_shared_utility/tls_config.h"

#ifdef USE_MBED_TLS

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

#include "xio.h"
#include "xlogging.h"
#include "optionhandler.h"

extern CONCRETE_IO_HANDLE tlsio_mbedtls_create(void* io_create_parameters);
extern void tlsio_mbedtls_destroy(CONCRETE_IO_HANDLE tls_io);
extern int tlsio_mbedtls_open(CONCRETE_IO_HANDLE tls_io, ON_IO_OPEN_COMPLETE on_io_open_complete, void* on_io_open_complete_context, ON_BYTES_RECEIVED on_bytes_received, void* on_bytes_received_context, ON_IO_ERROR on_io_error, void* on_io_error_context);
extern int tlsio_mbedtls_close(CONCRETE_IO_HANDLE tls_io, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* callback_context);
extern int tlsio_mbedtls_send(CONCRETE_IO_HANDLE tls_io, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context);
extern void tlsio_mbedtls_dowork(CONCRETE_IO_HANDLE tls_io);
extern int tlsio_mbedtls_setoption(CONCRETE_IO_HANDLE tls_io, const char* optionName, const void* value);
extern OPTIONHANDLER_HANDLE tlsio_mbedtls_retrieveoptions(CONCRETE_IO_HANDLE handle);

extern const IO_INTERFACE_DESCRIPTION* tlsio_mbedtls_get_interface_description(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* USE_MBED_TLS */

#endif /* TLSIO_MBEDTLS_H */

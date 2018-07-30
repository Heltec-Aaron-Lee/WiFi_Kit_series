// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_CLIENT_OPTIONS_H
#define IOTHUB_CLIENT_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct IOTHUB_PROXY_OPTIONS_TAG
    {
        const char* host_address;
        const char* username;
        const char* password;
    } IOTHUB_PROXY_OPTIONS;

    static const char* OPTION_LOG_TRACE = "logtrace";
    static const char* OPTION_X509_CERT = "x509certificate";
    static const char* OPTION_X509_PRIVATE_KEY = "x509privatekey";
    static const char* OPTION_KEEP_ALIVE = "keepalive";

    static const char* OPTION_PROXY_HOST = "proxy_address";
    static const char* OPTION_PROXY_USERNAME = "proxy_username";
    static const char* OPTION_PROXY_PASSWORD = "proxy_password";

    static const char* OPTION_SAS_TOKEN_LIFETIME = "sas_token_lifetime";
    static const char* OPTION_SAS_TOKEN_REFRESH_TIME = "sas_token_refresh_time";
    static const char* OPTION_CBS_REQUEST_TIMEOUT = "cbs_request_timeout";

    static const char* OPTION_MIN_POLLING_TIME = "MinimumPollingTime";
    static const char* OPTION_BATCHING = "Batching";

    static const char* OPTION_MESSAGE_TIMEOUT = "messageTimeout";
    static const char* OPTION_PRODUCT_INFO = "product_info";
    static const char* OPTION_C2D_KEEP_ALIVE_FREQ_SECS = "c2d_keep_alive_freq_secs";

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_OPTIONS_H */

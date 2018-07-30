// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SHARED_UTIL_OPTIONS_H
#define SHARED_UTIL_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct HTTP_PROXY_OPTIONS_TAG
    {
        const char* host_address;
        int port;
        const char* username;
        const char* password;
    } HTTP_PROXY_OPTIONS;

    static const char* OPTION_HTTP_PROXY = "proxy_data";
    static const char* OPTION_HTTP_TIMEOUT = "timeout";

    static const char* OPTION_TRUSTED_CERT = "TrustedCerts";

    static const char* SU_OPTION_X509_CERT = "x509certificate";
    static const char* SU_OPTION_X509_PRIVATE_KEY = "x509privatekey";

    static const char* OPTION_X509_ECC_CERT = "x509EccCertificate";
    static const char* OPTION_X509_ECC_KEY = "x509EccAliasKey";

    static const char* OPTION_CURL_LOW_SPEED_LIMIT = "CURLOPT_LOW_SPEED_LIMIT";
    static const char* OPTION_CURL_LOW_SPEED_TIME = "CURLOPT_LOW_SPEED_TIME";
    static const char* OPTION_CURL_FRESH_CONNECT = "CURLOPT_FRESH_CONNECT";
    static const char* OPTION_CURL_FORBID_REUSE = "CURLOPT_FORBID_REUSE";
    static const char* OPTION_CURL_VERBOSE = "CURLOPT_VERBOSE";

    static const char* OPTION_NET_INT_MAC_ADDRESS = "net_interface_mac_address";
#ifdef __cplusplus
}
#endif

#endif /* SHARED_UTIL_OPTIONS_H */

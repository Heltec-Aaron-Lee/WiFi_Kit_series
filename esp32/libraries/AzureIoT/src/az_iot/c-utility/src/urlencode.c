// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "az_iot/c-utility/inc/azure_c_shared_utility/gballoc.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/urlencode.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/xlogging.h"
#include "az_iot/c-utility/inc/azure_c_shared_utility/strings.h"

#define NIBBLE_STR(c) (char)(c < 10 ? c + '0' : c - 10 + 'a')
#define IS_PRINTABLE(c) (                           \
    (c == 0) ||                                     \
    (c == '!') ||                                   \
    (c == '(') || (c == ')') || (c == '*') ||       \
    (c == '-') || (c == '.') ||                     \
    ((c >= '0') && (c <= '9')) ||                   \
    ((c >= 'A') && (c <= 'Z')) ||                   \
    (c == '_') ||                                   \
    ((c >= 'a') && (c <= 'z'))                      \
)

static size_t URL_PrintableChar(unsigned char charVal, char* buffer)
{
    size_t size;
    if (IS_PRINTABLE(charVal))
    {
        buffer[0] = (char)charVal;
        size = 1;
    }
    else
    {
        char bigNibbleStr;
        char littleNibbleStr;
        unsigned char bigNibbleVal = charVal >> 4;
        unsigned char littleNibbleVal = charVal & 0x0F;

        if (bigNibbleVal >= 0x0C)
        {
            bigNibbleVal -= 0x04;
        }

        bigNibbleStr = NIBBLE_STR(bigNibbleVal);
        littleNibbleStr = NIBBLE_STR(littleNibbleVal);

        buffer[0] = '%';

        if (charVal < 0x80)
        {
            buffer[1] = bigNibbleStr;
            buffer[2] = littleNibbleStr;
            size = 3;
        }
        else
        {
            buffer[1] = 'c';
            buffer[3] = '%';
            buffer[4] = bigNibbleStr;
            buffer[5] = littleNibbleStr;
            if (charVal < 0xC0)
            {
                buffer[2] = '2';
            }
            else
            {
                buffer[2] = '3';
            }
            size = 6;
        }
    }

    return size;
}

static size_t URL_PrintableCharSize(unsigned char charVal)
{
    size_t size;
    if (IS_PRINTABLE(charVal))
    {
        size = 1;
    }
    else
    {
        if (charVal < 0x80)
        {
            size = 3;
        }
        else
        {
            size = 6;
        }
    }
    return size;
}

STRING_HANDLE URL_EncodeString(const char* textEncode)
{
    STRING_HANDLE result;
    if (textEncode == NULL)
    {
        result = NULL;
    }
    else
    {
        STRING_HANDLE tempString = STRING_construct(textEncode);
        if (tempString == NULL)
        {
            result = NULL;
        }
        else
        {
            result = URL_Encode(tempString);
            STRING_delete(tempString);
        }
    }
    return result;
}

STRING_HANDLE URL_Encode(STRING_HANDLE input)
{
    STRING_HANDLE result;
    if (input == NULL)
    {
        /*Codes_SRS_URL_ENCODE_06_001: [If input is NULL then URL_Encode will return NULL.]*/
        result = NULL;
        LogError("URL_Encode:: NULL input");
    }
    else
    {
        size_t lengthOfResult = 0;
        char* encodedURL;
        const char* currentInput;
        unsigned char currentUnsignedChar;
        currentInput = STRING_c_str(input);
        /*Codes_SRS_URL_ENCODE_06_003: [If input is a zero length string then URL_Encode will return a zero length string.]*/
        do
        {
            currentUnsignedChar = (unsigned char)(*currentInput++);
            lengthOfResult += URL_PrintableCharSize(currentUnsignedChar);
        } while (currentUnsignedChar != 0);
        if ((encodedURL = (char*)malloc(lengthOfResult)) == NULL)
        {
            /*Codes_SRS_URL_ENCODE_06_002: [If an error occurs during the encoding of input then URL_Encode will return NULL.]*/
            result = NULL;
            LogError("URL_Encode:: MALLOC failure on encode.");
        }
        else
        {
            size_t currentEncodePosition = 0;
            currentInput = STRING_c_str(input);
            do
            {
                currentUnsignedChar = (unsigned char)(*currentInput++);
                currentEncodePosition += URL_PrintableChar(currentUnsignedChar, &encodedURL[currentEncodePosition]);
            } while (currentUnsignedChar != 0);

            result = STRING_new_with_memory(encodedURL);
            if (result == NULL)
            {
                LogError("URL_Encode:: MALLOC failure on encode.");
                free(encodedURL);
            }
        }
    }
    return result;
}

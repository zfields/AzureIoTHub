// Copyright (c) Arduino. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <time.h>
#include <sys/time.h>

#include "httpapi.h"
#include "iot_logging.h"

#include <AzureIoTHub.h>
#include "util/HTTPSClient.h"


HTTPSClient* httpsClient = NULL;

HTTPAPI_RESULT HTTPAPI_Init(void)
{
    httpsClient = new HTTPSClient(AzureIoTHubClient::sslClient);
    httpsClient->setTimeout(10000);

    return HTTPAPI_OK;
}

void HTTPAPI_Deinit(void)
{
}

HTTP_HANDLE HTTPAPI_CreateConnection(const char* hostName)
{
    HTTPSClient* client = NULL;

    if (httpsClient->connected()) {
        // no clients available
        LogError("No HTTPS clients available\n");
    } else if (httpsClient->begin(hostName)) {
        client = httpsClient;
    } else {
        // connection failed
        LogError("HTTPS connection to %s failed\n", hostName);
    }

    return ((HTTP_HANDLE)client);
}

void HTTPAPI_CloseConnection(HTTP_HANDLE handle)
{
    HTTPSClient* client = (HTTPSClient*)handle;

    client->end();
}

static const char* HTTPRequestTypes[] = {
    "GET",
    "POST",
    "PUT",
    "DELETE",
    "PATCH"
};

HTTPAPI_RESULT HTTPAPI_ExecuteRequest(HTTP_HANDLE handle,
        HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
        HTTP_HEADERS_HANDLE httpHeadersHandle, const unsigned char* content,
        size_t contentLength, unsigned int* statusCode,
        HTTP_HEADERS_HANDLE responseHeadersHandle,
        BUFFER_HANDLE responseContent)
{
    int result;
    size_t headersCount;
    char* header;

    HTTPSClient* client = (HTTPSClient*)handle;

    if (!client->connected()) {
        // client not connected
        LogError("HTTPS request failed, client not connected\n");
        return HTTPAPI_OPEN_REQUEST_FAILED;
    }

    result = client->sendRequest(HTTPRequestTypes[requestType], relativePath);
    if (!result) {
        LogError("HTTPS send request failed\n");
        return HTTPAPI_SEND_REQUEST_FAILED;
    }

    HTTPHeaders_GetHeaderCount(httpHeadersHandle, &headersCount);

    for (size_t i = 0; i < headersCount && result; i++) {
        HTTPHeaders_GetHeader(httpHeadersHandle, i, &header);
        result = client->sendHeader(header);
        free(header);
    }

    if (!result) {
        LogError("HTTPS send header failed\n");
        return HTTPAPI_SEND_REQUEST_FAILED;
    }

    result = client->sendBody(content, contentLength);
    if (!result) {
        LogError("HTTPS send body failed\n");
        return HTTPAPI_SEND_REQUEST_FAILED;
    }

    result = client->readStatus();
    if (result == -1) {
        return HTTPAPI_STRING_PROCESSING_ERROR;
    }
    *statusCode = result;

    while (result > 0) {
        String headerName;
        String headerValue;

        result = client->readHeader(headerName, headerValue);
        HTTPHeaders_AddHeaderNameValuePair(responseHeadersHandle, headerName.c_str(), headerValue.c_str());
    }

    if (result == -1) {
        LogError("HTTPS header parsing failed\n");
        return HTTPAPI_HTTP_HEADERS_FAILED;
    }

    contentLength = client->contentLength();
    if (contentLength) {
        BUFFER_pre_build(responseContent, contentLength);
        client->readBody(BUFFER_u_char(responseContent), contentLength);
    }

    return HTTPAPI_OK;
}

HTTPAPI_RESULT HTTPAPI_SetOption(HTTP_HANDLE handle, const char* optionName,
        const void* value)
{
    HTTPSClient* client = (HTTPSClient*)handle;
    HTTPAPI_RESULT result = HTTPAPI_INVALID_ARG;

    if (strcmp("timeout", optionName) == 0) {
        client->setTimeout(*(const unsigned int*)value);

        result = HTTPAPI_OK;
    }

    return result;
}

HTTPAPI_RESULT HTTPAPI_CloneOption(const char* optionName, const void* value,
        const void** savedValue)
{
    HTTPAPI_RESULT result = HTTPAPI_INVALID_ARG;

    if (strcmp("timeout", optionName) == 0) {
        unsigned int* timeout = (unsigned int*)malloc(sizeof(unsigned int));

        *timeout = *(const unsigned int*)value;

        *savedValue = &timeout;

        result = HTTPAPI_OK;
    }

    return result;
}

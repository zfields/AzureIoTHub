// Copyright (c) Arduino. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef AZUREIOTHUB_H
#define AZUREIOTHUB_H

#include <sys/time.h>

#include "sdk/lock.h"
#include "sdk/platform.h"
#include "sdk/serializer.h"
#include "sdk/threadapi.h"

#include "sdk/iothub_client_ll.h"
#include "sdk/iothub_message.h"
#include "sdk/iothubtransporthttp.h"

#ifdef __cplusplus

#include "Client.h"

namespace AzureIoTHubClient {
    extern Client * sslClient;

    static int setEpochTime(const unsigned long epochTime)
    {
#ifdef ARDUINO_ARCH_ESP8266
        return -1;
#else
        struct timeval tv;

        tv.tv_sec = epochTime;
        tv.tv_usec = 0;

        return settimeofday(&tv, NULL);
#endif
    }
} // namespace AzureIoTHubClient

#endif

#endif

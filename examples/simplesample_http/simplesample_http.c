// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "simplesample_http.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* This sample uses the _LL APIs of iothub_client for example purposes.
That does not mean that HTTP only works with the _LL APIs.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

#ifdef ARDUINO
  #include "AzureIoTHub.h"
  #include "connection_settings.h"
#else
  #include "serializer.h"
  #include "iothub_client_ll.h"
  #include "iothubtransporthttp.h"
  #include "azure_c_shared_utility/threadapi.h"
  #include "azure_c_shared_utility/platform.h"
  
  /*String containing Hostname, Device Id & Device Key in the format:             */
  /*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"    */
  static const char* connectionString = "[device connection string]";
#endif

#ifdef MBED_BUILD_TIMESTAMP
  #include "certs.h"
#endif // MBED_BUILD_TIMESTAMP

// Define the Model
BEGIN_NAMESPACE(WeatherStation);

DECLARE_MODEL(ContosoAnemometer,
  WITH_DATA(ascii_char_ptr, DeviceId),
  WITH_DATA(int, WindSpeed),
  WITH_ACTION(TurnFanOn),
  WITH_ACTION(TurnFanOff),
  WITH_ACTION(SetAirResistance, int, Position)
);

END_NAMESPACE(WeatherStation);

DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES)

IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = NULL;
ContosoAnemometer* myWeather = NULL;

EXECUTE_COMMAND_RESULT TurnFanOn(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan on.\r\n");
    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT TurnFanOff(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan off.\r\n");
    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT SetAirResistance(ContosoAnemometer* device, int Position)
{
    (void)device;
    (void)printf("Setting Air Resistance Position to %d.\r\n", Position);
    return EXECUTE_COMMAND_SUCCESS;
}

void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    int messageTrackingId = (intptr_t)userContextCallback;

    (void)printf("Message Id: %d Received.\r\n", messageTrackingId);

    (void)printf("Result Call Back Called! Result is: %s \r\n", ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
    static unsigned int messageTrackingId;
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
    if (messageHandle == NULL)
    {
        (void)printf("unable to create a new IoTHubMessage\r\n");
    }
    else
    {
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)(uintptr_t)messageTrackingId) != IOTHUB_CLIENT_OK)
        {
            (void)printf("failed to hand over the message to IoTHubClient");
        }
        else
        {
            (void)printf("IoTHubClient accepted the message for delivery\r\n");
        }
        IoTHubMessage_Destroy(messageHandle);
    }
    free((void*)buffer);
    messageTrackingId++;
}

/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    const unsigned char* buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        (void)printf("unable to IoTHubMessage_GetByteArray\r\n");
        result = EXECUTE_COMMAND_ERROR;
    }
    else
    {
        /*buffer is not zero terminated*/
        char* temp = malloc(size + 1);
        if (temp == NULL)
        {
            (void)printf("failed to malloc\r\n");
            result = EXECUTE_COMMAND_ERROR;
        }
        else
        {
            EXECUTE_COMMAND_RESULT executeCommandResult;
        
            memcpy(temp, buffer, size);
            temp[size] = '\0';
            executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
            result =
                (executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
                (executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
                IOTHUBMESSAGE_REJECTED;
            free(temp);
        }
    }
    return result;
}

void sendModelFields (int parameterCount, ...) {
    va_list field_list;
    va_start(field_list, parameterCount);

    const int avgWindSpeed = 10;

    myWeather->DeviceId = "myFirstDevice";
    myWeather->WindSpeed = avgWindSpeed + (rand() % 4 + 2);
    
    unsigned char* serializedData;
    size_t serializedDataSize;
    
    if (SERIALIZE(&serializedData, &serializedDataSize, field_list) != IOT_AGENT_OK)
    {
        (void)printf("Failed to serialize\r\n");
        return;
    }
    
    va_end(field_list);

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(serializedData, serializedDataSize);
    if (messageHandle == NULL)
    {
        (void)printf("unable to create a new IoTHubMessage\r\n");
    }
    else
    {
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)1) != IOTHUB_CLIENT_OK)
        {
            (void)printf("failed to hand over the message to IoTHubClient");
        }
        else
        {
            (void)printf("IoTHubClient accepted the message for delivery\r\n");
        }

        IoTHubMessage_Destroy(messageHandle);
    }

    free(serializedData);
}

void simplesample_http_send_model_to_azure (void) {
    const int avgWindSpeed = 10;

    myWeather->DeviceId = "myFirstDevice";
    myWeather->WindSpeed = avgWindSpeed + (rand() % 4 + 2);
    
    unsigned char* serializedData;
    size_t serializedDataSize;
    
    if (SERIALIZE(&serializedData, &serializedDataSize, *myWeather) != IOT_AGENT_OK)
    {
        (void)printf("Failed to serialize\r\n");
        return;
    }

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(serializedData, serializedDataSize);
    if (messageHandle == NULL)
    {
        (void)printf("unable to create a new IoTHubMessage\r\n");
    }
    else
    {
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)1) != IOTHUB_CLIENT_OK)
        {
            (void)printf("failed to hand over the message to IoTHubClient");
        }
        else
        {
            (void)printf("IoTHubClient accepted the message for delivery\r\n");
        }

        IoTHubMessage_Destroy(messageHandle);
    }

    free(serializedData);
}

void simplesample_http_setup(void)
{
    // Initialize the specific platform
#ifdef ARDUINO
    if (platform_init_with_parameters((void *)connection_params) != 0)
#else
    if (platform_init() != 0)
#endif
    {
        (void)printf("Failed to initialize the platform.\r\n");
        return;
    }

    // Prepare the serializer library
    if (serializer_init(NULL) != SERIALIZER_OK)
    {
        (void)printf("Failed on serializer_init\r\n");
        platform_deinit();
        return;
    }

    // Obtain a handle to the lower-level (no thread handling) Azure IoTHub Client library
    iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);
    if (iotHubClientHandle == NULL)
    {
        (void)printf("Failed on IoTHubClient_LL_Create\r\n");
        serializer_deinit();
        platform_deinit();
        return;
    }

    // Set options on the Azure IoTHub Client to alter default behaviors
    
    // Because it can poll "after 9 seconds" polls will happen 
    // effectively at ~10 seconds.
    // NOTE: The default value of "MinimumPollingTime" is 25 minutes.
    // For more information, see:
    // https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging
    unsigned int minimumPollingTime = 9;

    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
    {
        (void)printf("failure to set option \"MinimumPollingTime\"\r\n");
    }
#ifdef MBED_BUILD_TIMESTAMP
    // For mbed add the certificate information
    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
    {
        (void)printf("failure to set option \"TrustedCerts\"\r\n");
    }
#endif // MBED_BUILD_TIMESTAMP

    // Create instance of data model
    myWeather = CREATE_MODEL_INSTANCE(WeatherStation, ContosoAnemometer);
    if (myWeather == NULL)
    {
        (void)printf("Failed on CREATE_MODEL_INSTANCE\r\n");
        IoTHubClient_LL_Destroy(iotHubClientHandle);
        serializer_deinit();
        platform_deinit();
        return;
    }

    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, IoTHubMessage, myWeather) != IOTHUB_CLIENT_OK)
    {
        (void)printf("unable to IoTHubClient_SetMessageCallback\r\n");
        DESTROY_MODEL_INSTANCE(myWeather);
        IoTHubClient_LL_Destroy(iotHubClientHandle);
        serializer_deinit();
        platform_deinit();
        return;
    }

    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Set base device info
    myWeather->DeviceId = "myFirstDevice";
}

void simplesample_http_background_work(void)
{
    IoTHubClient_LL_DoWork(iotHubClientHandle);
}

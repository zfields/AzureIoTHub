// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Use Arduino IDE 1.6.8 or later.

#include "simplesample_http.h"

int referenceTime = 0;

void setup() {
    // Initialize serial and wait for port to open
    Serial.begin(9600);
    
// To reenable diagnostic output from WiFi libraries on ESP8266 boards
#ifdef ARDUINO_ARCH_ESP8266
    Serial.setDebugOutput(true);
#endif

    while (!Serial); // wait for serial port to connect. Needed for native USB port only

    // Invoke the AzureIoTHubClient Loop
    simplesample_http_setup();
}

void loop() {
    int loopTime = millis();
    
    if ((loopTime - referenceTime) > 1000) {
        referenceTime = loopTime;
        simplesample_http_send_model_to_azure();
    }
    
    delay(100);
    simplesample_http_background_work();
}


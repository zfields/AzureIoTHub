// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "platform.h"

#if defined(ARDUINO) && ARDUINO >= 100
  //#include <Arduino.h>
#else
  //#include <WProgram.h>
#endif

// Azure IoT SDK libraries
#include <AzureIoTHub.h>

#ifdef ARDUINO_ARCH_ESP8266
  // for ESP8266
  #include <ESP8266WiFi.h>
  #include <WiFiClientSecure.h>
  #include <WiFiUdp.h>

  static WiFiClientSecure sslClient;
#elif ARDUINO_SAMD_FEATHER_M0
  // for Adafruit WINC1500
  #include <Adafruit_WINC1500.h>
  #include <Adafruit_WINC1500SSLClient.h>
  #include <Adafruit_WINC1500Udp.h>
  #include <NTPClient.h>
  #include <SPI.h>

  // For the Adafruit WINC1500 we need to create our own WiFi instance
  // Define the WINC1500 board connections below.
  #define WINC_CS   8
  #define WINC_IRQ  7
  #define WINC_RST  4
  #define WINC_EN   2     // or, tie EN to VCC

  // Setup the WINC1500 connection with the pins above and the default hardware SPI.
  static Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);
  static Adafruit_WINC1500SSLClient sslClient;
#else
  #include <WiFi101.h>
  #include <WiFiSSLClient.h>
  #include <WiFiUdp.h>
  #include <NTPClient.h>

  static WiFiSSLClient sslClient;
#endif

static void initTime (void);
static void initWifi (const char ssid[], const char pass[]);


int platform_init_with_parameters(void * parameters)
{
    if (NULL == parameters) {
        return -1;
    }

    char ** connection_params = (char **)parameters;

    initWifi(connection_params[0], connection_params[1]);
    initTime();

    //iotHubClient.sslClient = &sslClient;
    AzureIoTHubClient::sslClient = &sslClient;

    return 0;
}


int platform_init(void)
{
    return -1;
}


static void initTime() {
#if ARDUINO_ARCH_ESP8266
    time_t epochTime;

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true) {
        epochTime = time(NULL);

        if (epochTime == 0) {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else {
            Serial.print("Fetched NTP epoch time is: ");
            Serial.println(epochTime);
            break;
        }
    }
#else
  #ifdef ARDUINO_SAMD_FEATHER_M0
    Adafruit_WINC1500UDP ntpUdp; // for Adafruit WINC1500
  #else
    WiFiUDP ntpUdp;
  #endif
    NTPClient ntpClient(ntpUdp);

    ntpClient.begin();

    while (!ntpClient.update()) {
        Serial.println("Fetching NTP epoch time failed! Waiting 5 seconds to retry.");
        delay(5000);
    }

    ntpClient.end();

    unsigned long epochTime = ntpClient.getEpochTime();

    Serial.print("Fetched NTP epoch time is: ");
    Serial.println(epochTime);

    AzureIoTHubClient::setEpochTime(epochTime);
#endif
}


static void initWifi (const char ssid[], const char pass[]) {
#ifdef ARDUINO_SAMD_FEATHER_M0
    // for the Adafruit WINC1500 we need to enable the chip
    pinMode(WINC_EN, OUTPUT);
    digitalWrite(WINC_EN, HIGH);
#endif

    // check for the presence of the shield :
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
        // don't continue:
        while (true);
    }

    // attempt to connect to Wifi network:
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
        // unsuccessful, retry in 4 seconds
        Serial.print("failed ... ");
        delay(4000);
        Serial.print("retrying ... ");
    }

    Serial.println("Connected to wifi");
}


const IO_INTERFACE_DESCRIPTION* platform_get_default_tlsio(void)
{
    return NULL;
}


void platform_deinit(void)
{

}

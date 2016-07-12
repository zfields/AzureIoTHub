// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CONNECTION_SETTINGS_H
#define CONNECTION_SETTINGS_H

static const char ssid[] = "<wifi-ssid>";  // your network SSID (name)
static const char pass[] = "<wifi-password>";  // your network password (use for WPA, or use as key for WEP)

static const char *connection_params[] = {ssid, pass};

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char connectionString[] = "<azure-iothub-device-connection-string>";

#endif /* CONNECTION_SETTINGS_H */


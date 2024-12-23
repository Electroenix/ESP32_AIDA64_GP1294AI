#ifndef _WIFI_CLIENT_H_
#define _WIFI_CLIENT_H_
#include <WiFi.h>

#define wifiPrintLog(format, arg...) UARTPrintf("\r\n[WIFI] " format, ##arg)

extern void taskWifiClient(void *param);

#endif
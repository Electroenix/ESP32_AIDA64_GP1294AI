#ifndef _WIFI_CLIENT_H_
#define _WIFI_CLIENT_H_
#include <WiFi.h>
#include "trace.h"


typedef enum
{
    WIFI_STATUS_NOT_INIT = 0,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_CONNECT_FAILED,
    WIFI_STATUS_MAX,
} WIFI_STATUS;

typedef struct
{
    String ssid;
    int rssi;
    int encryption_type;
}wifi_scan_result_t;

typedef struct
{
    int status;
    bool is_ap_mode;
    std::vector<wifi_scan_result_t> last_wifi_scan_list;
}wifi_context_t;

extern wifi_context_t wifi_context;

void startWifiScan(void);
void taskWifiClient(void *param);
void initWifiClient(void);

#endif
#include "config.h"
#include "display.h"
#include "wifi_client.h"

typedef enum
{
    WIFI_NOT_INIT,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    //WIFI_DISCONNECT,
    //WIFI_RECONNECTING,
    WIFI_STATUS_MAX,
}WIFI_STATUS;

void taskWifiClient(void *param)
{
    bool ret = 0;
    unsigned long reconnectTick = 0;
    unsigned long connectBeginTick = 0;
    unsigned long connectingTick = 0;
    int wifiStatus = WIFI_NOT_INIT;

    Serial.print("[TASK] taskWifiClient run!\r\n");

    while(1)
    {
        if(WiFi.status() != WL_CONNECTED)
        {
            if(wifiStatus == WIFI_NOT_INIT)
            {
                wifiStatus = WIFI_CONNECTING;

                WiFi.begin(WIFI_SSID, WIFI_PASS);
                Serial.print("[WIFI] Connect to " WIFI_SSID "\r\n");
                display.clear();
                display.print("\r\nWIFI begin\r\n");
                display.print("\r\nConnect to\n" WIFI_SSID "\r\n");
                display.print("\r\nConnecting");
                connectBeginTick = millis();
            }

            if(wifiStatus == WIFI_CONNECTED)
            {
                wifiStatus = WIFI_CONNECTING;

                Serial.print("[WIFI] Disconnect!\r\n");
                Serial.print("[WIFI] Reconnect to " WIFI_SSID "\r\n");

                display.clear();
                display.print("WIFI Disconnect!\r\n");
                display.print("\r\nReconnecting");

                WiFi.reconnect();
                connectBeginTick = millis();
            }

            if(wifiStatus == WIFI_CONNECTING && getElapsedTick(reconnectTick) >= 15000)
            {
                Serial.print("[WIFI] Connect timeout!\r\n");
                Serial.print("[WIFI] Reconnect\r\n");

                display.clear();
                display.print("WIFI Connect failed!\r\n");
                display.print("\r\nReconnecting ");

                WiFi.reconnect();
                reconnectTick = millis();
            }

            if(wifiStatus == WIFI_CONNECTING && getElapsedTick(connectingTick) >= 1000)
            {
                Serial.print("[WIFI] Connecting...\r\n");

                display.print(".");
                connectingTick = millis();
            }

            if(wifiStatus == WIFI_CONNECTING && getElapsedTick(connectBeginTick) >= 60000)
            {
                u8g2.setPowerSave(1);//60s未连接关闭屏幕
                Serial.print("[VFD DISPLAY] PowerSave Mode\r\n");
            }
        }
        else
        {
            if(wifiStatus == WIFI_CONNECTING)
            {
                wifiStatus = WIFI_CONNECTED;
                Serial.print("[WIFI] Connect succeed!\r\n");

                u8g2.setPowerSave(0);
                display.print("\r\nSucceed!\r\n");
            }
        }

        delay(100);
    }
}

unsigned long getElapsedTick(unsigned long lastTick)
{
    return (millis() - lastTick);
}
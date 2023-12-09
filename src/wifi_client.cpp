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
    unsigned long ledTick = 0;
    int wifiStatus = WIFI_NOT_INIT;

    wifiPrintLog("taskWifiClient run!\r\n");

    while(1)
    {
        if(WiFi.status() != WL_CONNECTED)
        {
            if(wifiStatus == WIFI_NOT_INIT)
            {
                wifiStatus = WIFI_CONNECTING;

                WiFi.begin(WIFI_SSID, WIFI_PASS);
                wifiPrintLog("Connect to " WIFI_SSID "\r\n");
                display.clear();
                display.print("\r\nWIFI begin\r\n");
                display.print("\r\nConnect to\n" WIFI_SSID "\r\n");
                display.print("\r\nConnecting");
                connectBeginTick = millis();
            }

            if(wifiStatus == WIFI_CONNECTED)
            {
                wifiStatus = WIFI_CONNECTING;

                wifiPrintLog("Disconnect!\r\n");
                wifiPrintLog("Reconnect to " WIFI_SSID "\r\n");

                display.clear();
                display.print("WIFI Disconnect!\r\n");
                display.print("\r\nReconnecting");

                WiFi.reconnect();
                connectBeginTick = millis();
            }

            if(wifiStatus == WIFI_CONNECTING)
            {
                if(getElapsedTick(connectingTick) >= 1000)
                {
                    wifiPrintLog("Connecting...\r\n");

                    display.print(".");
                    connectingTick = millis();
                }

                if(getElapsedTick(reconnectTick) >= 15000)
                {
                    wifiPrintLog("Connect timeout!\r\n");
                    wifiPrintLog("Reconnect\r\n");

                    display.clear();
                    display.print("WIFI Connect failed!\r\n");
                    display.print("\r\nReconnecting ");

                    WiFi.reconnect();
                    reconnectTick = millis();
                }

                if(getElapsedTick(connectBeginTick) >= 60000)
                {
                    u8g2.setPowerSave(1);//60s未连接关闭屏幕
                    wifiPrintLog("Screen enter PowerSave Mode\r\n");
                }
            }
        }
        else
        {
            if(wifiStatus == WIFI_CONNECTING)
            {
                wifiStatus = WIFI_CONNECTED;
                wifiPrintLog("Wifi Connect succeed!\r\n");

                u8g2.setPowerSave(0);
                display.print("\r\nSucceed!\r\n");
            }
#if 0
            if(getElapsedTick(ledTick) > 3000)
            {
                pinMode(2, OUTPUT);
                digitalWrite(2, HIGH);
                delay(5);
                digitalWrite(2, LOW);
                ledTick = millis();
            }
#endif
        }

        delay(100);
    }
}

unsigned long getElapsedTick(unsigned long lastTick)
{
    return (millis() - lastTick);
}
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "display.h"
#include "wifi_client.h"

wifi_context_t wifi_context;

unsigned long getElapsedTick(unsigned long lastTick)
{
    return (millis() - lastTick);
}

// 启动AP模式
void startAPMode()
{
    char display_buf[128] = {0};

    ESP_LOGI(WIFI_TAG, "Enable AP Mode");

    WiFi.softAP(g_config.ap_auth_cfg.ssid, g_config.ap_auth_cfg.password);
    wifi_context.is_ap_mode = true;

    ESP_LOGI(WIFI_TAG, "AP SSID: %s", g_config.ap_auth_cfg.ssid.c_str());
    ESP_LOGI(WIFI_TAG, "AP PASSWD: %s", g_config.ap_auth_cfg.password.c_str());
    ESP_LOGI(WIFI_TAG, "AP IP: %s", WiFi.softAPIP().toString().c_str());
}

void stopAPMode()
{
    WiFi.softAPdisconnect(true);
    wifi_context.is_ap_mode = false;
}

void startWifiScan()
{
    ESP_LOGI(WIFI_TAG, "Start scanning network...");

    int n = WiFi.scanNetworks();
    ESP_LOGI(WIFI_TAG, "Total %d networks found", n);

    StaticJsonDocument<2048> doc;
    JsonArray networks = doc.to<JsonArray>();

    wifi_context.last_wifi_scan_list.clear();
    for (int i = 0; i < n; i++)
    {
        wifi_scan_result_t wifi_scan_result;
        wifi_scan_result.ssid = WiFi.SSID(i);
        wifi_scan_result.rssi = WiFi.RSSI(i);
        wifi_scan_result.encryption_type = WiFi.encryptionType(i);
        wifi_context.last_wifi_scan_list.push_back(wifi_scan_result);
        ESP_LOGI(WIFI_TAG, "[%d] ssid: %s | rssi: %d | encryptionType: %d", i, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.encryptionType(i));
    }

    WiFi.scanDelete();
}

void taskWifiClient(void *param)
{
    bool ret = 0;
    unsigned long reconnectTick = 0;
    unsigned long connectBeginTick = 0;
    unsigned long connectingTick = 0;
    unsigned long ledTick = 0;
    char displayBuf[128] = {0};

    ESP_LOGI(WIFI_TAG, "taskWifiClient run!");

    while (1)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            if (wifi_context.status == WIFI_STATUS_NOT_INIT)
            {
                // WiFi.mode(WIFI_STA);
                WiFi.mode(WIFI_AP_STA);
                WiFi.begin(g_config.sta_auth_cfg.ssid, g_config.sta_auth_cfg.password);
                ESP_LOGI(WIFI_TAG, "Connect to %s", g_config.sta_auth_cfg.ssid);

                wifi_context.status = WIFI_STATUS_CONNECTING;

                display.print("WIFI begin\r\n");
                sprintf(displayBuf, "Connect to %s\r\n", g_config.sta_auth_cfg.ssid);
                display.print(displayBuf);
                display.print("Connecting ");
                connectBeginTick = millis();
            }

            if (wifi_context.status == WIFI_STATUS_CONNECTED)
            {
                wifi_context.status = WIFI_STATUS_CONNECTING;

                ESP_LOGW(WIFI_TAG, "Disconnect!");
                ESP_LOGI(WIFI_TAG, "Reconnect to %s", g_config.sta_auth_cfg.ssid);

                display.print("WIFI Disconnect!\r\n");
                display.print("Reconnecting ");

                WiFi.reconnect();
                connectBeginTick = millis();
            }

            if (wifi_context.status == WIFI_STATUS_CONNECTING)
            {
                if (getElapsedTick(connectingTick) >= 1000)
                {
                    static LoadingString loading_str = LoadingString();
                    display.print(loading_str.step().c_str());
                    connectingTick = millis();
                }

                if (getElapsedTick(reconnectTick) >= 15000)
                {
                    ESP_LOGW(WIFI_TAG, "Connect timeout!");
                    ESP_LOGI(WIFI_TAG, "Reconnect");
                    display.print("\r\nfailed!\r\n");
                    display.print("Reconnecting ");

                    WiFi.reconnect();
                    reconnectTick = millis();
                }

                if (getElapsedTick(connectBeginTick) >= 60000)
                {
                    // 1min未连接成功，则切换AP模式，可以通过AP进入web界面修改配置
                    WiFi.disconnect();
                    startWifiScan();
                    // WiFi.mode(WIFI_AP);
                    startAPMode();
                    display.clear();
                    sprintf(displayBuf, "WiFi connect failed, Now you can connect this AP and website to modify config or reboot device\r\n");
                    display.print(displayBuf);
                    sprintf(displayBuf, "AP SSID: %s\r\n", g_config.ap_auth_cfg.ssid.c_str());
                    display.print(displayBuf);
                    sprintf(displayBuf, "AP PASSWD: %s\r\n", g_config.ap_auth_cfg.password.c_str());
                    display.print(displayBuf);
                    sprintf(displayBuf, "Web Address: http://%s", WiFi.softAPIP().toString().c_str());
                    display.print(displayBuf);

                    wifi_context.status = WIFI_STATUS_CONNECT_FAILED;
                }
            }

            if (wifi_context.status == WIFI_STATUS_CONNECT_FAILED)
            {
                if (getElapsedTick(connectBeginTick) >= 180000)
                {

                    if (!display.getPowerSaveMode())
                    {
                        display.setPowerSaveMode(1); // 3min未连接关闭屏幕
                        ESP_LOGI(WIFI_TAG, "Screen enter PowerSave Mode");
                    }
                }
            }
        }
        else
        {
            if (wifi_context.status == WIFI_STATUS_CONNECTING)
            {
                wifi_context.status = WIFI_STATUS_CONNECTED;
                ESP_LOGI(WIFI_TAG, "Wifi Connect succeed!");

                display.setPowerSaveMode(0);
                display.print("Succeed!\r\n");
                sprintf(displayBuf, "Web Address %s\r\n", WiFi.localIP().toString().c_str());
                display.print(displayBuf);
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

void initWifiContext()
{
    memset(&wifi_context, 0x00, sizeof(wifi_context));
    wifi_context.status = WIFI_STATUS_NOT_INIT;
    wifi_context.is_ap_mode = false;
}

void initWifiClient()
{
    initWifiContext();
    xTaskCreate(taskWifiClient, "taskWifiClient", 10240, NULL, 2, NULL);
}
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

// 保存配置到文件
bool saveWifiConfig(const wifi_auth_config_t &config)
{
    File file = LittleFS.open(WIFI_CONFIG_FILE, "w");
    if (!file)
    {
        ESP_LOGE(WIFI_TAG, "open file %s failed!", WIFI_CONFIG_FILE);
        return false;
    }

    StaticJsonDocument<256> doc;
    doc["ssid"] = config.ssid;
    doc["password"] = config.password;

    if (serializeJson(doc, file) == 0)
    {
        ESP_LOGE(WIFI_TAG, "write %s failed!", WIFI_CONFIG_FILE);
        file.close();
        return false;
    }

    file.close();
    ESP_LOGI(WIFI_TAG, "wifi config saved");
    return true;
}

// 从文件加载配置
bool loadWifiConfig(wifi_auth_config_t &config)
{
    if (!LittleFS.exists(WIFI_CONFIG_FILE))
    {
        ESP_LOGE(WIFI_TAG, "file %s not exist, use default config", WIFI_CONFIG_FILE);
        saveWifiConfig(config);
    }

    File file = LittleFS.open(WIFI_CONFIG_FILE, "r");
    if (!file)
    {
        ESP_LOGE(WIFI_TAG, "open file %s failed!", WIFI_CONFIG_FILE);
        return false;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        ESP_LOGE(WIFI_TAG, "deserializeJson failed");
        return false;
    }

    config.ssid = doc["ssid"] | "";
    config.password = doc["password"] | "";

    ESP_LOGI(WIFI_TAG, "wifi config loaded");
    return true;
}

// 启动AP模式
void startAPMode()
{
    char display_buf[128] = {0};

    ESP_LOGI(WIFI_TAG, "Enable AP Mode");

    WiFi.softAP(wifi_context.ap_auth_cfg.ssid, wifi_context.ap_auth_cfg.password);
    wifi_context.is_ap_mode = true;

    ESP_LOGI(WIFI_TAG, "AP SSID: %s", wifi_context.ap_auth_cfg.ssid.c_str());
    ESP_LOGI(WIFI_TAG, "AP PASSWD: %s", wifi_context.ap_auth_cfg.password.c_str());
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
                loadWifiConfig(wifi_context.sta_auth_cfg);
                WiFi.begin(wifi_context.sta_auth_cfg.ssid, wifi_context.sta_auth_cfg.password);
                ESP_LOGI(WIFI_TAG, "Connect to %s", wifi_context.sta_auth_cfg.ssid);

                wifi_context.status = WIFI_STATUS_CONNECTING;

                display.print("WIFI begin\r\n");
                sprintf(displayBuf, "Connect to %s\r\n", wifi_context.sta_auth_cfg.ssid);
                display.print(displayBuf);
                display.print("Connecting");
                connectBeginTick = millis();
            }

            if (wifi_context.status == WIFI_STATUS_CONNECTED)
            {
                wifi_context.status = WIFI_STATUS_CONNECTING;

                ESP_LOGW(WIFI_TAG, "Disconnect!");
                ESP_LOGI(WIFI_TAG, "Reconnect to %s", wifi_context.sta_auth_cfg.ssid);

                display.print("WIFI Disconnect!\r\n");
                display.print("Reconnecting");

                WiFi.reconnect();
                connectBeginTick = millis();
            }

            if (wifi_context.status == WIFI_STATUS_CONNECTING)
            {
                if (getElapsedTick(connectingTick) >= 1000)
                {
                    ESP_LOGI(WIFI_TAG, "Connecting...");

                    display.print(".");
                    connectingTick = millis();
                }

                if (getElapsedTick(reconnectTick) >= 15000)
                {
                    ESP_LOGW(WIFI_TAG, "Connect timeout!");
                    ESP_LOGI(WIFI_TAG, "Reconnect");
                    display.print("WIFI Connect failed!\r\n");
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
                    sprintf(displayBuf, "AP SSID: %s\r\n", wifi_context.ap_auth_cfg.ssid.c_str());
                    display.print(displayBuf);
                    sprintf(displayBuf, "AP PASSWD: %s\r\n", wifi_context.ap_auth_cfg.password.c_str());
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
    wifi_context.sta_auth_cfg.ssid = WIFI_SSID;
    wifi_context.sta_auth_cfg.password = WIFI_PASSWORD;
    wifi_context.ap_auth_cfg.ssid = AP_SSID;
    wifi_context.ap_auth_cfg.password = AP_PASSWORD;
}

void initWifiClient()
{
    initWifiContext();
    xTaskCreate(taskWifiClient, "taskWifiClient", 10240, NULL, 2, NULL);
}
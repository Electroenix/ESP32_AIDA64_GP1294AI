#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "trace.h"


config_t g_config;

// 保存配置到文件
bool saveConfig(const config_t &config)
{
    File file = LittleFS.open(CONFIG_FILE_PATH, "w");
    if (!file)
    {
        ESP_LOGE(MAIN_TAG, "open file %s failed!", CONFIG_FILE_PATH);
        return false;
    }

    StaticJsonDocument<512> doc;

    doc["sta_ssid"] = config.sta_auth_cfg.ssid;
    doc["sta_password"] = config.sta_auth_cfg.password;
    doc["ap_ssid"] = config.ap_auth_cfg.ssid;
    doc["ap_password"] = config.ap_auth_cfg.password;
    doc["host_ip"] = config.host_ip;
    doc["host_port"] = config.host_port;

    if (serializeJson(doc, file) == 0) {
        ESP_LOGE(MAIN_TAG, "Failed to write to file");
        file.close();
        return false;
    }

    file.close();
    ESP_LOGI(MAIN_TAG, "system config saved");
    return true;
}

// 从文件加载配置
bool loadConfig(config_t &config)
{
    if (!LittleFS.exists(CONFIG_FILE_PATH))
    {
        ESP_LOGE(MAIN_TAG, "file %s not exist, use default config", CONFIG_FILE_PATH);
        saveConfig(config);
    }

    File file = LittleFS.open(CONFIG_FILE_PATH, "r");
    if (!file)
    {
        ESP_LOGE(MAIN_TAG, "open file %s failed!", CONFIG_FILE_PATH);
        return false;
    }
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        ESP_LOGE(MAIN_TAG, "Failed to parse config file: %s", error.c_str());
        return false;
    }

    config.sta_auth_cfg.ssid = doc["sta_ssid"].as<String>();
    config.sta_auth_cfg.password = doc["sta_password"].as<String>();
    config.ap_auth_cfg.ssid = doc["ap_ssid"].as<String>();
    config.ap_auth_cfg.password = doc["ap_password"].as<String>();
    config.host_ip = doc["host_ip"].as<String>();
    config.host_port = doc["host_port"];

    return true;
}

void loadDefaultConfig(config_t &config)
{
    config.sta_auth_cfg.ssid = WIFI_SSID;
    config.sta_auth_cfg.password = WIFI_PASSWORD;
    config.ap_auth_cfg.ssid = AP_SSID;
    config.ap_auth_cfg.password = AP_PASSWORD;
    config.host_ip = HOST_IP;
    config.host_port = HOST_PORT;
}

void initConfig()
{
    loadDefaultConfig(g_config);
    loadConfig(g_config);

    ESP_LOGI(WIFI_TAG, "system config loaded");
    ESP_LOGI(MAIN_TAG, "sta_auth_cfg.ssid: %s", g_config.sta_auth_cfg.ssid.c_str());
    ESP_LOGI(MAIN_TAG, "sta_auth_cfg.password: %s", g_config.sta_auth_cfg.password.c_str());
    ESP_LOGI(MAIN_TAG, "ap_auth_cfg.ssid : %s", g_config.ap_auth_cfg.ssid.c_str());
    ESP_LOGI(MAIN_TAG, "ap_auth_cfg.password: %s", g_config.ap_auth_cfg.password.c_str());
    ESP_LOGI(MAIN_TAG, "host_ip: %s", g_config.host_ip.c_str());
    ESP_LOGI(MAIN_TAG, "host_port: %d", g_config.host_port);
}


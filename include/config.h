#ifndef _CONFIG_H_
#define _CONFIG_H_

/* 
 * GP1294AI PIN define
 * use 3 wire SPI
 * SCL -> 18
 * SDA -> 23
 */
#define CS 5
#define RST 21

//WIFI
#define WIFI_SSID "wifi_ssid"
#define WIFI_PASSWORD "wifi_password"

//AP
#define AP_SSID     "Esp32-AP-" + String(ESP.getEfuseMac() & 0xFFFFFF, HEX)
#define AP_PASSWORD "12345678"

//主机AIDA64软件开启的tcp地址和端口
#define HOST_IP "HOST_IP"
#define HOST_PORT 8080

#define CONFIG_FILE_PATH     "/config.json"

typedef struct
{
    String ssid;
    String password;
}wifi_auth_config_t;

typedef struct
{
    wifi_auth_config_t sta_auth_cfg;
    wifi_auth_config_t ap_auth_cfg;
    String host_ip;
    uint16_t host_port;
}config_t;

extern config_t g_config;

bool saveConfig(const config_t &config);
bool loadConfig(config_t &config);
void initConfig(void);
#endif
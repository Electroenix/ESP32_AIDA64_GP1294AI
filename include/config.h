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

//HTTP
#define HTTP_HOST "192.168.100.146"
#define HTTP_PORT 8080

#endif
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "trace.h"
#include "wifi_client.h"
#include "config.h"

WebServer server(80);
String wifiScanRspCache;

// API: 获取系统状态
void handleStatus() {
  StaticJsonDocument<256> doc;

  if (wifi_context.is_ap_mode) {
    doc["wifi_status"] = "ap_mode";
    doc["ssid"] = WiFi.softAPSSID();
    doc["ip"] = WiFi.softAPIP().toString();
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      doc["wifi_status"] = "connected";
      doc["ssid"] = WiFi.SSID();
      doc["ip"] = WiFi.localIP().toString();
      doc["rssi"] = WiFi.RSSI();
    } else {
      doc["wifi_status"] = "disconnected";
    }
  }
  
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["chip_id"] = String(ESP.getEfuseMac() & 0xFFFFFFFF, HEX);

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// API: 扫描WiFi网络
void handleScanNetworks() {
  StaticJsonDocument<2048> doc;
  JsonArray networks = doc.to<JsonArray>();

  String body = server.arg("plain");
  StaticJsonDocument<256> docBody;
  DeserializationError error = deserializeJson(docBody, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }

  //判断是否读取上一次缓存
  if(docBody["read_cache"])
  {
    if(wifi_context.last_wifi_scan_list.size() <= 0)
    {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"No cache wifiscan list\"}");
      return;
    }
  }
  else
  {
    startWifiScan();
  }

  for (int i = 0; i < wifi_context.last_wifi_scan_list.size(); i++) {
    JsonObject network = networks.createNestedObject();
    network["ssid"] = wifi_context.last_wifi_scan_list[i].ssid;
    network["rssi"] = wifi_context.last_wifi_scan_list[i].rssi;
    network["encryption"] = (wifi_context.last_wifi_scan_list[i].encryption_type == WIFI_AUTH_OPEN) ? "open" : "encrypted";
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
  ESP_LOGI(WEB_SERVER_TAG, "response > %s", response.c_str());
}

// API: 保存配置
void handleSaveConfig() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }
  
  String body = server.arg("plain");
  ESP_LOGI(WEB_SERVER_TAG, "body: %s", body.c_str());

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }

  config_t newConfig;
  memcpy(&newConfig, &g_config, sizeof(newConfig));
  newConfig.sta_auth_cfg.ssid = doc["ssid"] | "";
  newConfig.sta_auth_cfg.password = doc["password"] | "";
  newConfig.host_ip = doc["host_ip"] | "";
  newConfig.host_port = doc["host_port"];

  if (newConfig.sta_auth_cfg.ssid.isEmpty()) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"SSID is required\"}");
    return;
  }

  if (saveConfig(newConfig)) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Config saved\"}");
    
    // 重启应用配置
    delay(1000);
    ESP.restart();
  } else {
    server.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save config\"}");
  }
}

// API: 重启设备
void handleRestart() {
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Restarting...\"}");
  delay(1000);
  ESP.restart();
}

// API: 恢复出厂设置
void handleReset() {
  if (LittleFS.remove(CONFIG_FILE_PATH)) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Config reset\"}");
    delay(1000);
    ESP.restart();
  } else {
    server.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to reset config\"}");
  }
}

// 静态文件服务
void handleStaticFile() {
  String path = server.uri();
  
  if (path.endsWith("/")) {
    path += "index.html";
  }

  String contentType = "text/plain";
  if (path.endsWith(".html")) contentType = "text/html";
  else if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";
  else if (path.endsWith(".json")) contentType = "application/json";
  
  ESP_LOGI(WEB_SERVER_TAG, "path: %s", path);
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

void taskWebServer(void *param)
{
  while(1)
  {
    server.handleClient();
    delay(10);
  }
}

void initWebServer()
{
  while(wifi_context.status == WIFI_STATUS_NOT_INIT)
  {
      delay(1000);
  }

  // 设置API路由
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/wifi/scan", HTTP_POST, handleScanNetworks);
  server.on("/api/wifi/config", HTTP_POST, handleSaveConfig);
  server.on("/api/system/restart", HTTP_POST, handleRestart);
  server.on("/api/system/reset", HTTP_POST, handleReset);

  // 静态文件服务
  server.onNotFound(handleStaticFile);

  server.begin();
  ESP_LOGI(WEB_SERVER_TAG, "HTTP Server Running");

  xTaskCreate(taskWebServer, "taskWebServer", 10240, NULL, 2, NULL);
}
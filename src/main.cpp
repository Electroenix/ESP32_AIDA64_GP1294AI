#include <Arduino.h>
#include "http_client.h"
#include "display.h"
#include "config.h"
#include "wifi_client.h"
#include "trace.h"
#include "web_server.h"
#include "file_system.h"

/* default config */
int screen_dir = SCREEN_DIR_HORIZONTAL;

void setup()
{
    // put your setup code here, to run once:
    initLogger();
    ESP_LOGI(MAIN_TAG, "Initial start...");
    initFileSystem();
    initDisplay();
    initWifiClient();
    initWebServer();
    initHttpClient();
}

void loop()
{
    // put your main code here, to run repeatedly:
#if 0
    delay(1000);
#endif
}

